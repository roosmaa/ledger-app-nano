/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2018 Mart Roosmaa
*   (c) 2016 Ledger
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

#include "coins.h"
#include "libn_internal.h"
#include "libn_apdu_get_address.h"
#include "libn_apdu_constants.h"

#include "libn_bagl.h"

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_UNUSED 0x00

uint16_t libn_apdu_get_address_output(libn_apdu_response_t *resp, libn_apdu_get_address_request_t *req);

uint16_t libn_apdu_get_address(libn_apdu_response_t *resp) {
    libn_apdu_get_address_request_t req;
    libn_private_key_t privateKey;

    uint8_t *keyPathPtr;
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_NO_DISPLAY:
    case P1_DISPLAY:
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

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return LIBN_SW_INCORRECT_LENGTH;
    }
    keyPathPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;

    if (!os_global_pin_is_validated()) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (display && libn_context_D.state != LIBN_STATE_READY) {
        return LIBN_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Configure the formatter
    libn_address_formatter_for_coin(&req.addressFormatter, COIN_DEFAULT_PREFIX, keyPathPtr);

    // Retrieve the public key for the path
    libn_derive_keypair(keyPathPtr, privateKey, req.publicKey);
    os_memset(privateKey, 0, sizeof(privateKey)); // sanitise private key

    if (display) {
        // Update app state to confirm the address
        libn_context_D.state = LIBN_STATE_CONFIRM_ADDRESS;
        os_memmove(&libn_context_D.stateData.getAddressRequest, &req, sizeof(req));
        os_memset(&req, 0, sizeof(req)); // sanitise request data
        app_apply_state();

        resp->ioFlags |= IO_ASYNCH_REPLY;
        return LIBN_SW_OK;
    } else {
        uint16_t statusWord = libn_apdu_get_address_output(resp, &req);
        os_memset(&req, 0, sizeof(req)); // sanitise request data
        return statusWord;
    }
}

uint16_t libn_apdu_get_address_output(libn_apdu_response_t *resp, libn_apdu_get_address_request_t *req) {
    uint8_t length;
    uint8_t *outPtr = resp->buffer;

    // Output raw public key
    length = sizeof(req->publicKey);
    os_memmove(outPtr, req->publicKey, length);
    outPtr += length;

    // Encode & output account address
    length = libn_address_format(&req->addressFormatter, outPtr + 1, req->publicKey);
    *outPtr = length;
    outPtr += 1 + length;

    resp->outLength = outPtr - resp->buffer;

    return LIBN_SW_OK;
}

void libn_bagl_display_address_callback(bool confirmed) {
    libn_apdu_get_address_request_t *req = &libn_context_D.stateData.getAddressRequest;

    uint16_t statusWord;
    libn_apdu_response_t resp;
    resp.buffer = libn_async_buffer_D;
    resp.outLength = 0;
    resp.ioFlags = 0;

    if (confirmed) {
        statusWord = libn_apdu_get_address_output(&resp, req);
    } else {
        statusWord = LIBN_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    os_memset(req, 0, sizeof(req)); // sanitise request data
    app_async_response(&resp, statusWord);
}
