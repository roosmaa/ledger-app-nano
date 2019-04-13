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
#include "libn_apdu_cache_block.h"
#include "libn_bagl.h"

#define P1_UNUSED 0x00
#define P2_UNUSED 0x00

uint16_t libn_apdu_cache_block_output(libn_apdu_response_t *resp, libn_apdu_cache_block_request_t *req);

uint16_t libn_apdu_cache_block(libn_apdu_response_t *resp) {
    libn_apdu_cache_block_request_t req;
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    libn_private_key_t privateKey;
    uint8_t *inPtr;
    uint8_t readLen;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_UNUSED:
        break;
    default:
        return LIBN_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_UNUSED:
        break;
    default:
        return LIBN_SW_INCORRECT_P1_P2;
    }

    // Verify the minimum size
    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 177) {
        return LIBN_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    readLen = 1 + (*inPtr) * 4;
    os_memmove(keyPath, inPtr, MIN(readLen, sizeof(keyPath)));
    inPtr += readLen;

    if (!os_global_pin_is_validated()) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (libn_context_D.state != LIBN_STATE_READY) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Derive public key for hashing
    libn_derive_keypair(keyPath, privateKey, req.publicKey);
    os_memset(privateKey, 0, sizeof(privateKey)); // sanitise private key
    os_memset(&keyPath, 0, sizeof(keyPath));

    // Reset block state
    os_memset(&req.block, 0, sizeof(req.block));

    // Parse input data
    readLen = sizeof(req.block.parent);
    os_memmove(req.block.parent, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(req.block.link);
    os_memmove(req.block.link, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(req.block.representative);
    os_memmove(req.block.representative, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(req.block.balance);
    os_memmove(req.block.balance, inPtr, readLen);
    inPtr += readLen;

    readLen = sizeof(req.signature);
    os_memmove(req.signature, inPtr, readLen);
    inPtr += readLen;

    libn_hash_block(req.blockHash, &req.block, req.publicKey);

    uint16_t statusWord = libn_apdu_cache_block_output(resp, &req);
    os_memset(&req, 0, sizeof(req)); // sanitise request data
    return statusWord;
}

uint16_t libn_apdu_cache_block_output(libn_apdu_response_t *resp, libn_apdu_cache_block_request_t *req) {
    // Copy the data over to the cache
    os_memset(&libn_context_D.cachedBlock, 0, sizeof(libn_context_D.cachedBlock));
    os_memmove(libn_context_D.cachedBlock.representative, req->block.representative,
        sizeof(libn_context_D.cachedBlock.representative));
    os_memmove(libn_context_D.cachedBlock.balance, req->block.balance,
        sizeof(libn_context_D.cachedBlock.balance));
    os_memmove(libn_context_D.cachedBlock.hash, req->blockHash,
        sizeof(libn_context_D.cachedBlock.hash));

    return LIBN_SW_OK;
}
