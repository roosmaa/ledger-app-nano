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
#include "nano_apdu_constants.h"

#include "nano_bagl.h"

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_UNUSED 0x00

uint16_t nano_apdu_get_address_output(void);

uint16_t nano_apdu_get_address() {
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

    // Retrieve the public key for the path
    nano_private_derive_keypair(keyPathPtr, true);
    os_memset(nano_private_key_D, 0, sizeof(nano_private_key_D)); // sanitise private key

    if (display) {
        nano_context_D.ioFlags |= IO_ASYNCH_REPLY;
        nano_bagl_display_address();
        return NANO_SW_OK;
    } else {
        return nano_apdu_get_address_output();
    }
}

uint16_t nano_apdu_get_address_output(void) {
    uint8_t length;
    uint8_t *outPtr = G_io_apdu_buffer;

    // Output raw public key
    length = sizeof(nano_public_key_D);
    *outPtr = length;
    os_memmove(outPtr + 1, nano_public_key_D, length);
    outPtr += 1 + length;

    // Encode & output account address
    length = NANO_ACCOUNT_STRING_BASE_LEN + NANO_DEFAULT_PREFIX_LEN;
    *outPtr = length;
    nano_write_account_string(outPtr + 1, NANO_DEFAULT_PREFIX, nano_public_key_D);
    outPtr += 1 + length;

    nano_context_D.outLength = outPtr - G_io_apdu_buffer;

    // Reset the global variables
    os_memset(nano_public_key_D, 0, sizeof(nano_public_key_D));

    return NANO_SW_OK;
}

void nano_bagl_display_address_callback(bool confirmed) {
    nano_context_D.outLength = 0;
    nano_context_D.ioFlags = 0;

    if (confirmed) {
        nano_context_D.sw = nano_apdu_get_address_output();
    } else {
        nano_context_D.sw = NANO_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    app_async_response();
}
