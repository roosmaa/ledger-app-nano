/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2018 Mart Roosmaa
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "nano_internal.h"
#include "nano_apdu_constants.h"
#include "nano_apdu_sign_block.h"
#include "nano_bagl.h"

#ifdef HAVE_U2F

#include "u2f_service.h"
#include "u2f_transport.h"

extern bool fidoActivated;
extern volatile u2f_service_t u2fService;
void u2f_proxy_response(u2f_service_t *service, uint16_t tx);

#endif

#define P1_OPEN_BLOCK    0x00
#define P1_RECEIVE_BLOCK 0x01
#define P1_SEND_BLOCK    0x02
#define P1_CHANGE_BLOCK  0x03

#define P2_UNUSED 0x00

uint16_t nano_apdu_sign_block_output(nano_apdu_response_t *resp, nano_apdu_sign_block_request *req);

uint16_t nano_apdu_sign_block(nano_apdu_response_t *resp) {
    nano_apdu_sign_block_request req;
    nano_private_key_t privateKey;
    uint8_t *inPtr;
    uint8_t readLen;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
    case P1_RECEIVE_BLOCK:
    case P1_SEND_BLOCK:
    case P1_CHANGE_BLOCK:
        break;
    default:
        return NANO_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_UNUSED:
        break;
    default:
        return NANO_SW_INCORRECT_P1_P2;
    }

    // Verify the minimum size
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 34) {
            return NANO_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_RECEIVE_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 65) {
            return NANO_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_SEND_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 50) {
            return NANO_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_CHANGE_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 34) {
            return NANO_SW_INCORRECT_LENGTH;
        }
        break;
    default:
        return NANO_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    readLen = 1 + (*inPtr) * 4;
    os_memmove(req.keyPath, inPtr, MIN(readLen, sizeof(req.keyPath)));
    inPtr += readLen;

    if (!os_global_pin_is_validated()) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (nano_context_D.state != NANO_STATE_READY) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Derive public keys for hashing
    nano_private_derive_keypair(req.keyPath, privateKey, req.publicKey);
    os_memset(privateKey, 0, sizeof(privateKey)); // sanitise private key

    // Reset block state
    os_memset(&req.block, 0, sizeof(req.block));

    // Parse input data
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
        req.block.open.type = NANO_OPEN_BLOCK;

        readLen = *inPtr;
        if (!nano_read_account_string(
                inPtr + 1, readLen,
                &req.block.open.representativePrefix,
                req.block.open.representative)) {
            return NANO_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(req.block.open.sourceBlock);
        os_memmove(req.block.open.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_RECEIVE_BLOCK:
        req.block.receive.type = NANO_RECEIVE_BLOCK;

        readLen = sizeof(req.block.receive.previousBlock);
        os_memmove(req.block.receive.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = sizeof(req.block.receive.sourceBlock);
        os_memmove(req.block.receive.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_SEND_BLOCK:
        req.block.send.type = NANO_SEND_BLOCK;

        readLen = sizeof(req.block.send.previousBlock);
        os_memmove(req.block.send.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!nano_read_account_string(
                inPtr + 1, readLen,
                &req.block.send.destinationAccountPrefix,
                req.block.send.destinationAccount)) {
            return NANO_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(req.block.send.balance);
        os_memmove(req.block.send.balance, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_CHANGE_BLOCK:
        req.block.change.type = NANO_CHANGE_BLOCK;

        readLen = sizeof(req.block.change.previousBlock);
        os_memmove(req.block.change.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!nano_read_account_string(
                inPtr + 1, readLen,
                &req.block.change.representativePrefix,
                req.block.change.representative)) {
            return NANO_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;
        break;
    }

    nano_hash_block(&req.block, req.publicKey);

    // When auto receive is enabled, skip the prompt
    if (N_nano.autoReceive && req.block.base.type == NANO_RECEIVE_BLOCK) {
        uint16_t statusWord = nano_apdu_sign_block_output(resp, &req);
        os_memset(&req, 0, sizeof(req)); // sanitise request data
        return statusWord;
    } else {
        // Update app state to confirm the address
        nano_context_D.state = NANO_STATE_CONFIRM_SIGNATURE;
        os_memmove(&nano_context_D.stateData.signBlockRequest, &req, sizeof(req));
        app_apply_state();

        resp->ioFlags |= IO_ASYNCH_REPLY;
        return NANO_SW_OK;
    }
}

uint16_t nano_apdu_sign_block_output(nano_apdu_response_t *resp, nano_apdu_sign_block_request *req) {
    nano_private_key_t privateKey;
    uint8_t *outPtr = resp->buffer;

    // Derive key and sign the block
    nano_private_derive_keypair(req->keyPath, privateKey, NULL);
    nano_sign_block(&req->block, privateKey, req->publicKey);
    os_memset(privateKey, 0, sizeof(privateKey));

    // Output block hash
    os_memmove(outPtr, req->block.base.hash, sizeof(req->block.base.hash));
    outPtr += sizeof(req->block.base.hash);

    // Output signature
    os_memmove(outPtr, req->block.base.signature, sizeof(req->block.base.signature));
    outPtr += sizeof(req->block.base.signature);

    resp->outLength = outPtr - resp->buffer;

    return NANO_SW_OK;
}

void nano_bagl_confirm_sign_block_callback(bool confirmed) {
    nano_apdu_sign_block_request *req = &nano_context_D.stateData.signBlockRequest;

    uint16_t statusWord;
    nano_apdu_response_t resp;
    resp.buffer = nano_context_D.asyncBuffer;
    resp.outLength = 0;
    resp.ioFlags = 0;

    if (confirmed) {
        statusWord = nano_apdu_sign_block_output(&resp, req);
    } else {
        statusWord = NANO_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    os_memset(req, 0, sizeof(req)); // sanitise request data
    app_async_response(&resp, statusWord);
}
