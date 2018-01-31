/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
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

#define P1_UNUSED 0x00

#define P2_UNUSED 0x00

uint16_t nano_apdu_get_app_conf_output(void);

uint16_t nano_apdu_get_app_conf() {
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_UNUSED:
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

    if (G_io_apdu_buffer[ISO_OFFSET_LC] > 0x00) {
        return NANO_SW_INCORRECT_LENGTH;
    }

    return nano_apdu_get_app_conf_output();
}

uint16_t nano_apdu_get_app_conf_output(void) {
    uint8_t *outPtr = G_io_apdu_buffer;

    // Output raw public key
    *outPtr = APP_MAJOR_VERSION;
    outPtr += 1;
    *outPtr = APP_MINOR_VERSION;
    outPtr += 1;
    *outPtr = APP_PATCH_VERSION;
    outPtr += 1;

    nano_context_D.outLength = outPtr - G_io_apdu_buffer;

    return NANO_SW_OK;
}
