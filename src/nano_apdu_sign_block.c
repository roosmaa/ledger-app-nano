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

#define P1_UNUSED 0x00

#define P2_RECIPIENT_XRB_FLAG 0x01
#define P2_REPRESENTATIVE_XRB_FLAG 0x02

uint16_t nano_apdu_sign_block_output(nano_apdu_response_t *resp, nano_apdu_sign_block_request_t *req);

uint16_t nano_apdu_sign_block(nano_apdu_response_t *resp) {
    nano_apdu_sign_block_heap_t *h = &ram_a.nano_apdu_sign_block_heap_D;
    uint8_t *inPtr;
    uint8_t readLen;
    bool representativeChanged;
    bool balanceDecreased;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_UNUSED:
        break;
    default:
        return NANO_SW_INCORRECT_P1_P2;
    }

    // Verify the minimum size
    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 113) {
        return NANO_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    readLen = 1 + (*inPtr) * 4;
    os_memmove(h->req.keyPath, inPtr, MIN(readLen, sizeof(h->req.keyPath)));
    inPtr += readLen;

    if (!os_global_pin_is_validated()) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (nano_context_D.state != NANO_STATE_READY) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Store address display format preferences
    if ((G_io_apdu_buffer[ISO_OFFSET_P2] & P2_RECIPIENT_XRB_FLAG) != 0) {
        h->req.recipientPrefix = NANO_XRB_PREFIX;
    } else {
        h->req.recipientPrefix = NANO_DEFAULT_PREFIX;
    }
    if ((G_io_apdu_buffer[ISO_OFFSET_P2] & P2_REPRESENTATIVE_XRB_FLAG) != 0) {
        h->req.representativePrefix = NANO_XRB_PREFIX;
    } else {
        h->req.representativePrefix = NANO_DEFAULT_PREFIX;
    }

    // Derive public key for hashing
    nano_derive_keypair(h->req.keyPath, h->privateKey, h->req.publicKey);
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

    nano_hash_block(h->req.hash, &h->block, h->req.publicKey);

    // Determine changes that we've been requested to sign
    bool isFirstBlock = nano_is_zero(h->block.parent, sizeof(h->block.parent));
    if (isFirstBlock) {
        representativeChanged = true;
        os_memmove(h->req.representative, h->block.representative,
                sizeof(h->block.representative));
        // For first block the balance must have increased
        balanceDecreased = false;
        os_memmove(h->req.amount, h->block.balance, sizeof(h->req.amount));

    } else {
        // Make sure that the parent block data is cached and available
        if (os_memcmp(h->block.parent,
                      nano_context_D.cachedBlock.hash,
                      sizeof(h->block.parent)) != 0) {
            return NANO_SW_PARENT_BLOCK_CACHE_MISS;
        }

        representativeChanged = os_memcmp(
            h->block.representative,
            nano_context_D.cachedBlock.representative,
            sizeof(h->block.representative)) != 0;
        if (representativeChanged) {
            os_memmove(h->req.representative, h->block.representative,
                sizeof(h->block.representative));
        } else {
            os_memset(h->req.representative, 0,
                sizeof(h->block.representative));
        }

        balanceDecreased = nano_amount_cmp(
            h->block.balance,
            nano_context_D.cachedBlock.balance) < 0;
        if (balanceDecreased) {
            os_memmove(h->req.amount, nano_context_D.cachedBlock.balance, sizeof(h->req.amount));
            nano_amount_subtract(h->req.amount, h->block.balance);
        } else {
            os_memmove(h->req.amount, h->block.balance, sizeof(h->req.amount));
            nano_amount_subtract(h->req.amount, nano_context_D.cachedBlock.balance);
        }
    }

    if (balanceDecreased) {
        os_memmove(h->req.recipient, h->block.link,
            sizeof(h->req.recipient));
    } else {
        os_memset(h->req.recipient, 0,
            sizeof(h->req.recipient));
    }

    // When auto receive is enabled, skip the prompt
    if (N_nano.autoReceive && !balanceDecreased && !representativeChanged) {
        uint16_t statusWord = nano_apdu_sign_block_output(resp, &h->req);
        os_memset(&h->req, 0, sizeof(h->req)); // sanitise request data
        return statusWord;
    } else {
        // Update app state to confirm the address
        nano_context_D.state = NANO_STATE_CONFIRM_SIGNATURE;
        os_memmove(&nano_context_D.stateData.signBlockRequest, &h->req, sizeof(h->req));
        os_memset(&h->req, 0, sizeof(h->req)); // sanitise request data
        app_apply_state();

        resp->ioFlags |= IO_ASYNCH_REPLY;
        return NANO_SW_OK;
    }
}

uint16_t nano_apdu_sign_block_output(nano_apdu_response_t *resp, nano_apdu_sign_block_request_t *req) {
    nano_apdu_sign_block_output_heap_t *h = &ram_a.nano_apdu_sign_block_output_heap_D;
    uint8_t *outPtr = resp->buffer;

    // Derive key and sign the block
    nano_derive_keypair(req->keyPath, h->privateKey, NULL);
    nano_sign_block(h->signature, req->hash, h->privateKey, req->publicKey);
    os_memset(h->privateKey, 0, sizeof(h->privateKey));

    // Output block hash
    os_memmove(outPtr, req->hash, sizeof(req->hash));
    outPtr += sizeof(req->hash);

    // Output signature
    os_memmove(outPtr, h->signature, sizeof(h->signature));
    outPtr += sizeof(h->signature);

    resp->outLength = outPtr - resp->buffer;

    return NANO_SW_OK;
}

void nano_bagl_confirm_sign_block_callback(bool confirmed) {
    nano_apdu_sign_block_request_t *req = &nano_context_D.stateData.signBlockRequest;

    uint16_t statusWord;
    nano_apdu_response_t resp;
    resp.buffer = nano_async_buffer_D;
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
