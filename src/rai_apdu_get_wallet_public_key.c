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

#include "rai_bagl_extensions.h"

#ifdef HAVE_U2F

#include "u2f_service.h"
#include "u2f_transport.h"

extern bool fidoActivated;
extern volatile u2f_service_t u2fService;
void u2f_proxy_response(u2f_service_t *service, uint16_t tx);

#endif

#define P1_NO_DISPLAY 0x00
#define P1_DISPLAY 0x01

#define P2_UNUSED 0x00

uint16_t rai_apdu_get_wallet_public_key() {
    uint8_t *outPtr;
    uint8_t keyLength;
    uint8_t addressLength;
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    uint8_t chainCode[32];
    bool display = (G_io_apdu_buffer[ISO_OFFSET_P1] == P1_DISPLAY);

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_NO_DISPLAY:
        break;
    case P1_DISPLAY: // TODO: Implement support for display
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_UNUSED:
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

    rai_private_derive_keypair(keyPath, true, chainCode);
    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D)); // sanitise private key

    outPtr = G_io_apdu_buffer;

    // Output raw public key
    keyLength = sizeof(rai_public_key_D);
    *outPtr = keyLength;
    os_memmove(outPtr + 1, rai_public_key_D, keyLength);
    outPtr += 1 + keyLength;

    // Encode & output account address
    addressLength = ACCOUNT_STRING_LEN;
    *outPtr = addressLength;
    rai_write_account_string(outPtr + 1, rai_public_key_D);
    outPtr += 1 + addressLength;

    // Output chain code
    os_memmove(outPtr, chainCode, sizeof(chainCode));
    outPtr += sizeof(chainCode);

    rai_context_D.outLength = outPtr - G_io_apdu_buffer;

    if (display) {
        // TODO: implement display
    }

    return RAI_SW_OK;
}
