/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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

#include "rai_internal.h"
#include "rai_apdu_constants.h"

#include "rai_bagl.h"

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_NO_CHAINCODE 0x00
#define P2_CHAINCODE 0x01

uint16_t rai_apdu_get_address_output(void);

uint16_t rai_apdu_get_address() {
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);
    bool returnChainCode = G_io_apdu_buffer[ISO_OFFSET_P2] == P2_CHAINCODE;

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_NO_DISPLAY:
    case P1_DISPLAY:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_NO_CHAINCODE:
    case P2_CHAINCODE:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return RAI_SW_INCORRECT_LENGTH;
    }
    os_memmove(keyPath, G_io_apdu_buffer + ISO_OFFSET_CDATA,
               MAX_BIP32_PATH_LENGTH);

    if (!os_global_pin_is_validated()) {
        return RAI_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Retrieve the public key for the path
    rai_private_derive_keypair(keyPath, true, rai_context_D.chainCode);
    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D)); // sanitise private key
    if (!returnChainCode) {
        os_memmove(rai_context_D.chainCode, 0, sizeof(rai_context_D.chainCode));
    }

    if (display) {
        rai_context_D.ioFlags |= IO_ASYNCH_REPLY;
        rai_bagl_display_address();
        return RAI_SW_OK;
    } else {
        return rai_apdu_get_address_output();
    }
}

uint16_t rai_apdu_get_address_output(void) {
    uint8_t length;
    bool returnChainCode = G_io_apdu_buffer[ISO_OFFSET_P2] == P2_CHAINCODE;
    uint8_t *outPtr = G_io_apdu_buffer;

    // Output raw public key
    length = sizeof(rai_public_key_D);
    *outPtr = length;
    os_memmove(outPtr + 1, rai_public_key_D, length);
    outPtr += 1 + length;

    // Encode & output account address
    length = ACCOUNT_STRING_LEN;
    *outPtr = length;
    rai_write_account_string(outPtr + 1, rai_public_key_D);
    outPtr += 1 + length;

    // Output chain code
    if (returnChainCode) {
        os_memmove(outPtr, rai_context_D.chainCode, sizeof(rai_context_D.chainCode));
        outPtr += sizeof(rai_context_D.chainCode);
    }

    rai_context_D.outLength = outPtr - G_io_apdu_buffer;

    // Reset the global variables
    os_memset(rai_public_key_D, 0, sizeof(rai_public_key_D));
    if (returnChainCode) {
        os_memset(rai_context_D.chainCode, 0, sizeof(rai_context_D.chainCode));
    }

    return RAI_SW_OK;
}

void rai_bagl_display_address_callback(bool confirmed) {
    if (confirmed) {
        rai_context_D.sw = rai_apdu_get_address_output();
    } else {
        rai_context_D.sw = RAI_SW_CONDITIONS_OF_USE_NOT_SATISFIED;
    }
    app_async_response();
}
