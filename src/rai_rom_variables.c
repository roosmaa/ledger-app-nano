/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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

uint8_t const DISPATCHER_CLA[] = {
    RAI_CLA, // rai_apdu_get_wallet_public_key,
    RAI_CLA, // rai_apdu_sign_block,
    RAI_CLA, // rai_apdu_get_firmware_version,
};

uint8_t const DISPATCHER_INS[] = {
    RAI_INS_GET_WALLET_PUBLIC_KEY,    // rai_apdu_get_wallet_public_key,
    RAI_INS_SIGN_BLOCK,               // rai_apdu_sign_block
    RAI_INS_GET_FIRMWARE_VERSION,     // rai_apdu_get_firmware_version,
};

bool const DISPATCHER_DATA_IN[] = {
    true,  // rai_apdu_get_wallet_public_key,
    true,  // rai_apdu_sign_block
    false, // rai_apdu_get_firmware_version,
};

apduProcessingFunction const DISPATCHER_FUNCTIONS[] = {
    rai_apdu_get_wallet_public_key,
    rai_apdu_sign_block,
    rai_apdu_get_firmware_version,
};

uint8_t const BASE32_ALPHABET[32] = {
    '1', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'm', 'n', 'o', 'p', 'q',
    'r', 's', 't', 'u', 'w', 'x', 'y', 'z' };

uint8_t const BASE32_TABLE[75] = {
    0xff, 0x00, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x08, 0x09, 0x0a,
    0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0xff, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0xff, 0x1c,
    0x1d, 0x1e, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
    0xff, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
    0xff, 0x1c, 0x1d, 0x1e, 0x1f };
