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

#include "nano_internal.h"
#include "nano_apdu_get_address.h"
#include "nano_apdu_constants.h"

#include "nano_bagl.h"

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_UNUSED 0x00

uint16_t nano_apdu_get_address_output(nano_apdu_response_t *resp, nano_apdu_get_address_request_t *req);

uint16_t nano_apdu_get_address(nano_apdu_response_t *resp) {
    nano_apdu_get_address_heap_t *h = &nano_memory_space_a_D.nano_apdu_get_address_heap;
    uint8_t *keyPathPtr;
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_NO_DISPLAY:
    case P1_DISPLAY:
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

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return NANO_SW_INCORRECT_LENGTH;
    }
    keyPathPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;

    if (!os_global_pin_is_validated()) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }
    // Make sure that we're not about to interrupt another operation
    if (display && nano_context_D.state != NANO_STATE_READY) {
        return NANO_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Retrieve the public key for the path
    nano_derive_keypair(keyPathPtr, h->privateKey, h->req.publicKey);
    os_memset(h->privateKey, 0, sizeof(h->privateKey)); // sanitise private key

    if (display) {
        // Update app state to confirm the address
        nano_context_D.state = NANO_STATE_CONFIRM_ADDRESS;
        os_memmove(&nano_context_D.stateData.getAddressRequest, &h->req, sizeof(h->req));
        os_memset(&h->req, 0, sizeof(h->req)); // sanitise request data
        app_apply_state();

        resp->ioFlags |= IO_ASYNCH_REPLY;
        return NANO_SW_OK;
    } else {
        uint16_t statusWord = nano_apdu_get_address_output(resp, &h->req);
        os_memset(&h->req, 0, sizeof(h->req)); // sanitise request data
        return statusWord;
    }
}

uint16_t nano_apdu_get_address_output(nano_apdu_response_t *resp, nano_apdu_get_address_request_t *req) {
    uint8_t length;
    uint8_t *outPtr = resp->buffer;

    // Output raw public key
    length = sizeof(req->publicKey);
    *outPtr = length;
    os_memmove(outPtr + 1, req->publicKey, length);
    outPtr += 1 + length;

    // Encode & output account address
    length = NANO_ACCOUNT_STRING_BASE_LEN + NANO_DEFAULT_PREFIX_LEN;
    *outPtr = length;
    nano_write_account_string(outPtr + 1, NANO_DEFAULT_PREFIX, req->publicKey);
    outPtr += 1 + length;

    resp->outLength = outPtr - resp->buffer;

    return NANO_SW_OK;
}

void nano_bagl_display_address_callback(bool confirmed) {
    nano_apdu_get_address_request_t *req = &nano_context_D.stateData.getAddressRequest;

    uint16_t statusWord;
    nano_apdu_response_t resp;
    resp.buffer = nano_async_buffer_D;
    resp.outLength = 0;
    resp.ioFlags = 0;

    if (confirmed) {
        statusWord = nano_apdu_get_address_output(&resp, req);
    } else {
        statusWord = NANO_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    os_memset(req, 0, sizeof(req)); // sanitise request data
    app_async_response(&resp, statusWord);
}
