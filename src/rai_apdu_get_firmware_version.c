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

#define FEATURES_SELF_SCREEN_BUTTONS 0x02
#define FEATURES_EXTERNAL_SCREEN_BUTTONS 0x04
#define FEATURES_NFC 0x08
#define FEATURES_BLE 0x10
#define FEATURES_TEE 0x20

// Java Card is 0x60
#define ARCH_ID 0x30

uint16_t rai_apdu_get_firmware_version() {
    G_io_apdu_buffer[0] = FEATURES_SELF_SCREEN_BUTTONS;
    G_io_apdu_buffer[0] |= FEATURES_NFC;
    G_io_apdu_buffer[0] |= FEATURES_BLE;

    G_io_apdu_buffer[1] = ARCH_ID;
    G_io_apdu_buffer[2] = LEDGER_MAJOR_VERSION;
    G_io_apdu_buffer[3] = LEDGER_MINOR_VERSION;
    G_io_apdu_buffer[4] = LEDGER_PATCH_VERSION;

    rai_context_D.outLength = 5;

    return RAI_SW_OK;
}
