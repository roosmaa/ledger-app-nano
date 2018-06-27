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

#include "libn_internal.h"
#include "libn_apdu_constants.h"
#include "libn_apdu_get_address.h"
#include "libn_apdu_get_app_conf.h"
#include "libn_apdu_cache_block.h"
#include "libn_apdu_sign_block.h"
#include "libn_apdu_sign_nonce.h"

uint8_t const DISPATCHER_CLA[] = {
    LIBN_CLA, // libn_apdu_get_app_conf
    LIBN_CLA, // libn_apdu_get_address
    LIBN_CLA, // libn_apdu_cache_block
    LIBN_CLA, // libn_apdu_sign_block
    LIBN_CLA, // libn_apdu_sign_nonce
};

uint8_t const DISPATCHER_INS[] = {
    LIBN_INS_GET_APP_CONF,   // libn_apdu_get_app_conf
    LIBN_INS_GET_ADDRESS,    // libn_apdu_get_address
    LIBN_INS_CACHE_BLOCK,    // libn_apdu_cache_block
    LIBN_INS_SIGN_BLOCK,     // libn_apdu_sign_block
    LIBN_INS_SIGN_NONCE,     // libn_apdu_sign_nonce
};

bool const DISPATCHER_DATA_IN[] = {
    false, // libn_apdu_get_app_conf
    true,  // libn_apdu_get_address
    true,  // libn_apdu_cache_block
    true,  // libn_apdu_sign_block
    true,  // libn_apdu_sign_nonce
};

apduProcessingFunction const DISPATCHER_FUNCTIONS[] = {
    libn_apdu_get_app_conf,
    libn_apdu_get_address,
    libn_apdu_cache_block,
    libn_apdu_sign_block,
    libn_apdu_sign_nonce,
};

uint8_t const BASE16_ALPHABET[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F' };

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

uint8_t const BLOCK_HASH_PREAMBLE[32] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
};

uint8_t const NONCE_PREAMBLE[15] = " Signed Nonce:\n";
