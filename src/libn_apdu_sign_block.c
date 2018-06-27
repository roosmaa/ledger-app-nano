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

#include "libn_internal.h"
#include "libn_apdu_constants.h"
#include "libn_apdu_sign_block.h"
#include "libn_bagl.h"

#define P1_UNUSED 0x00

#define P2_RECIPIENT_XRB_FLAG 0x01
#define P2_REPRESENTATIVE_XRB_FLAG 0x02

uint16_t libn_apdu_sign_block_output(libn_apdu_response_t *resp, libn_apdu_sign_block_request_t *req);

uint16_t libn_apdu_sign_block(libn_apdu_response_t *resp) {
    libn_apdu_sign_block_request_t *req = &ram_a.libn_apdu_sign_block_heap_D.req;
    libn_apdu_sign_block_heap_input_t *h = &ram_a.libn_apdu_sign_block_heap_D.io.input;
    uint8_t *inPtr;
    uint8_t readLen;
    bool representativeChanged;
    bool balanceDecreased;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_UNUSED:
        break;
    default:
        return LIBN_SW_INCORRECT_P1_P2;
    }

    // Verify the minimum size
    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 113) {
        return LIBN_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    readLen = 1 + (*inPtr) * 4;
    os_memmove(req->keyPath, inPtr, MIN(readLen, sizeof(req->keyPath)));
    inPtr += readLen;

    if (!os_global_pin_is_validated()) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (libn_context_D.state != LIBN_STATE_READY) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Store address display format preferences
    if ((G_io_apdu_buffer[ISO_OFFSET_P2] & P2_RECIPIENT_XRB_FLAG) != 0) {
        req->recipientPrefix = LIBN_XRB_PREFIX;
    } else {
        req->recipientPrefix = LIBN_NANO_PREFIX;
    }
    if ((G_io_apdu_buffer[ISO_OFFSET_P2] & P2_REPRESENTATIVE_XRB_FLAG) != 0) {
        req->representativePrefix = LIBN_XRB_PREFIX;
    } else {
        req->representativePrefix = LIBN_NANO_PREFIX;
    }

    // Derive public key for hashing
    libn_derive_keypair(req->keyPath, h->privateKey, req->publicKey);
    os_memset(h->privateKey, 0, sizeof(h->privateKey)); // sanitise private key

    // Reset block state
    os_memset(&h->block, 0, sizeof(h->block));

    // Parse input data
    readLen = sizeof(h->block.parent);
    os_memmove(h->block.parent, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(h->block.link);
    os_memmove(h->block.link, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(h->block.representative);
    os_memmove(h->block.representative, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(h->block.balance);
    os_memmove(h->block.balance, inPtr, readLen);
    inPtr += readLen;

    libn_hash_block(req->blockHash, &h->block, req->publicKey);

    // Determine changes that we've been requested to sign
    bool isFirstBlock = libn_is_zero(h->block.parent, sizeof(h->block.parent));
    if (isFirstBlock) {
        representativeChanged = true;
        os_memmove(req->representative, h->block.representative,
                sizeof(h->block.representative));
        // For first block the balance must have increased
        balanceDecreased = false;
        os_memmove(req->amount, h->block.balance, sizeof(req->amount));

    } else {
        // Make sure that the parent block data is cached and available
        if (os_memcmp(h->block.parent,
                      libn_context_D.cachedBlock.hash,
                      sizeof(h->block.parent)) != 0) {
            return LIBN_SW_PARENT_BLOCK_CACHE_MISS;
        }

        representativeChanged = os_memcmp(
            h->block.representative,
            libn_context_D.cachedBlock.representative,
            sizeof(h->block.representative)) != 0;
        if (representativeChanged) {
            os_memmove(req->representative, h->block.representative,
                sizeof(h->block.representative));
        } else {
            os_memset(req->representative, 0,
                sizeof(h->block.representative));
        }

        balanceDecreased = libn_amount_cmp(
            h->block.balance,
            libn_context_D.cachedBlock.balance) < 0;
        if (balanceDecreased) {
            os_memmove(req->amount, libn_context_D.cachedBlock.balance, sizeof(req->amount));
            libn_amount_subtract(req->amount, h->block.balance);
        } else {
            os_memmove(req->amount, h->block.balance, sizeof(req->amount));
            libn_amount_subtract(req->amount, libn_context_D.cachedBlock.balance);
        }
    }

    if (balanceDecreased) {
        os_memmove(req->recipient, h->block.link,
            sizeof(req->recipient));
    } else {
        os_memset(req->recipient, 0,
            sizeof(req->recipient));
    }

    // When auto receive is enabled, skip the prompt
    if (N_nano.autoReceive && !balanceDecreased && !representativeChanged) {
        uint16_t statusWord = libn_apdu_sign_block_output(resp, req);
        os_memset(req, 0, sizeof(*req)); // sanitise request data
        return statusWord;
    } else {
        // Update app state to confirm the address
        libn_context_D.state = LIBN_STATE_CONFIRM_SIGNATURE;
        os_memmove(&libn_context_D.stateData.signBlockRequest, req, sizeof(*req));
        os_memset(req, 0, sizeof(*req)); // sanitise request data
        app_apply_state();

        resp->ioFlags |= IO_ASYNCH_REPLY;
        return LIBN_SW_OK;
    }
}

uint16_t libn_apdu_sign_block_output(libn_apdu_response_t *resp, libn_apdu_sign_block_request_t *req) {
    libn_apdu_sign_block_heap_output_t *h = &ram_a.libn_apdu_sign_block_heap_D.io.output;
    uint8_t *outPtr = resp->buffer;

    // Derive key and sign the block
    libn_derive_keypair(req->keyPath, h->privateKey, NULL);
    libn_sign_hash(h->signature, req->blockHash, h->privateKey, req->publicKey);
    os_memset(h->privateKey, 0, sizeof(h->privateKey));

    // Output block hash
    os_memmove(outPtr, req->blockHash, sizeof(req->blockHash));
    outPtr += sizeof(req->blockHash);

    // Output signature
    os_memmove(outPtr, h->signature, sizeof(h->signature));
    outPtr += sizeof(h->signature);

    resp->outLength = outPtr - resp->buffer;

    return LIBN_SW_OK;
}

void libn_bagl_confirm_sign_block_callback(bool confirmed) {
    libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;

    uint16_t statusWord;
    libn_apdu_response_t resp;
    resp.buffer = libn_async_buffer_D;
    resp.outLength = 0;
    resp.ioFlags = 0;

    if (confirmed) {
        statusWord = libn_apdu_sign_block_output(&resp, req);
    } else {
        statusWord = LIBN_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    os_memset(req, 0, sizeof(req)); // sanitise request data
    app_async_response(&resp, statusWord);
}
