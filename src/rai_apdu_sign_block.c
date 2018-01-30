/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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

#include "rai_internal.h"
#include "rai_apdu_constants.h"

#include "rai_bagl.h"

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

uint16_t rai_apdu_sign_block_output(void);

uint16_t rai_apdu_sign_block() {
    uint8_t *inPtr;
    uint8_t *keyPathPtr;
    uint8_t readLen;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
    case P1_RECEIVE_BLOCK:
    case P1_SEND_BLOCK:
    case P1_CHANGE_BLOCK:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_UNUSED:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    // Verify the minimum size
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 34) {
            return RAI_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_RECEIVE_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 65) {
            return RAI_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_SEND_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 50) {
            return RAI_SW_INCORRECT_LENGTH;
        }
        break;
    case P1_CHANGE_BLOCK:
        if (G_io_apdu_buffer[ISO_OFFSET_LC] < 34) {
            return RAI_SW_INCORRECT_LENGTH;
        }
        break;
    default:
        return RAI_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    keyPathPtr = inPtr;
    inPtr += 1 + (*inPtr) * 4;

    if (!os_global_pin_is_validated()) {
        return RAI_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Derive public keys for hashing
    rai_private_derive_keypair(keyPathPtr, true, rai_context_D.chainCode);
    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D)); // sanitise private key
    os_memset(rai_context_D.chainCode, 0, sizeof(rai_context_D.chainCode));

    // Reset block state
    os_memset(&rai_context_D.block, 0, sizeof(rai_context_D.block));

    // Parse input data
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
        rai_context_D.block.open.type = RAI_OPEN_BLOCK;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.open.representative)) {
            return RAI_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(rai_context_D.block.open.sourceBlock);
        os_memmove(rai_context_D.block.open.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_RECEIVE_BLOCK:
        rai_context_D.block.receive.type = RAI_RECEIVE_BLOCK;

        readLen = sizeof(rai_context_D.block.receive.previousBlock);
        os_memmove(rai_context_D.block.receive.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = sizeof(rai_context_D.block.receive.sourceBlock);
        os_memmove(rai_context_D.block.receive.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_SEND_BLOCK:
        rai_context_D.block.send.type = RAI_SEND_BLOCK;

        readLen = sizeof(rai_context_D.block.send.previousBlock);
        os_memmove(rai_context_D.block.send.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.send.destinationAccount)) {
            return RAI_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(rai_context_D.block.send.balance);
        os_memmove(rai_context_D.block.send.balance, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_CHANGE_BLOCK:
        rai_context_D.block.change.type = RAI_CHANGE_BLOCK;

        readLen = sizeof(rai_context_D.block.change.previousBlock);
        os_memmove(rai_context_D.block.change.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.change.representative)) {
            return RAI_SW_INCORRECT_DATA;
        }
        inPtr += 1 + readLen;
        break;
    }

    rai_hash_block(&rai_context_D.block);

    // When auto receive is enabled, skip the prompt
    if (N_rai.autoReceive && rai_context_D.block.base.type == RAI_RECEIVE_BLOCK) {
        return rai_apdu_sign_block_output();
    } else {
        rai_context_D.ioFlags |= IO_ASYNCH_REPLY;
        rai_bagl_confirm_sign_block();
        return RAI_SW_OK;
    }
}

uint16_t rai_apdu_sign_block_output(void) {
    uint8_t *keyPathPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    uint8_t *outPtr = G_io_apdu_buffer;

    // Derive key and sign the block
    rai_private_derive_keypair(keyPathPtr, false, rai_context_D.chainCode);
    os_memset(rai_context_D.chainCode, 0, sizeof(rai_context_D.chainCode));
    rai_sign_block(&rai_context_D.block);
    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D));

    // Output block hash
    os_memmove(outPtr, rai_context_D.block.base.hash, sizeof(rai_context_D.block.base.hash));
    outPtr += sizeof(rai_context_D.block.base.hash);

    // Output signature
    os_memmove(outPtr, rai_context_D.block.base.signature, sizeof(rai_context_D.block.base.signature));
    outPtr += sizeof(rai_context_D.block.base.signature);

    rai_context_D.outLength = outPtr - G_io_apdu_buffer;

    // Reset the global variables
    os_memset(rai_public_key_D, 0, sizeof(rai_public_key_D));

    return RAI_SW_OK;
}

void rai_bagl_confirm_sign_block_callback(bool confirmed) {
    if (confirmed) {
        rai_context_D.sw = rai_apdu_sign_block_output();
    } else {
        rai_context_D.sw = RAI_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    app_async_response();
}
