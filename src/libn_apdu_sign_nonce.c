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
#include "libn_apdu_sign_nonce.h"

#define P1_UNUSED 0x00
#define P2_UNUSED 0x00

uint16_t libn_apdu_sign_nonce_output(libn_apdu_response_t *resp, libn_apdu_sign_nonce_request_t *req);

uint16_t libn_apdu_sign_nonce(libn_apdu_response_t *resp) {
    libn_apdu_sign_nonce_request_t req;
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
    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 17) {
        return LIBN_SW_INCORRECT_LENGTH;
    }

    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    readLen = 1 + (*inPtr) * 4;
    os_memmove(req.keyPath, inPtr, MIN(readLen, sizeof(req.keyPath)));
    inPtr += readLen;

    if (!os_global_pin_is_validated()) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    readLen = sizeof(req.nonce);
    os_memmove(req.nonce, inPtr, readLen);
    inPtr += readLen;

    uint16_t statusWord = libn_apdu_sign_nonce_output(resp, &req);
    os_memset(&req, 0, sizeof(req)); // sanitise request data
    return statusWord;
}

uint16_t libn_apdu_sign_nonce_output(libn_apdu_response_t *resp, libn_apdu_sign_nonce_request_t *req) {
    libn_private_key_t privateKey;
    libn_public_key_t publicKey;
    libn_signature_t signature;
    uint8_t *outPtr = resp->buffer;

    // Derive key and sign the block
    libn_derive_keypair(req->keyPath, privateKey, publicKey);
    libn_sign_nonce(signature, req->nonce, privateKey, publicKey);
    os_memset(privateKey, 0, sizeof(privateKey));

    // Output signature
    os_memmove(outPtr, signature, sizeof(signature));
    outPtr += sizeof(signature);

    resp->outLength = outPtr - resp->buffer;

    return LIBN_SW_OK;
}
