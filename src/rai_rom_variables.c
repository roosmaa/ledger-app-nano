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
    RAI_CLA, // rai_apdu_get_firmware_version,
};

uint8_t const DISPATCHER_INS[] = {
    RAI_INS_GET_WALLET_PUBLIC_KEY,    // rai_apdu_get_wallet_public_key,
    RAI_INS_GET_FIRMWARE_VERSION,     // rai_apdu_get_firmware_version,
};

uint8_t const DISPATCHER_DATA_IN[] = {
    1, // rai_apdu_get_wallet_public_key,
    0, // rai_apdu_get_firmware_version,
};

apduProcessingFunction const DISPATCHER_FUNCTIONS[] = {
    rai_apdu_get_wallet_public_key,
    rai_apdu_get_firmware_version,
};