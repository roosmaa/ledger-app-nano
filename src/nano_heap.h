/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2018 Mart Roosmaa
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

#ifndef NANO_HEAP_H

#define NANO_HEAP_H

#include "nano_types.h"
#include "nano_apdu_get_address.h"
#include "nano_apdu_sign_block.h"

typedef struct {
    nano_apdu_get_address_request_t req;
    nano_private_key_t privateKey;
} nano_apdu_get_address_heap_t;

typedef struct {
    nano_apdu_sign_block_request_t req;
    nano_private_key_t privateKey;
} nano_apdu_sign_block_heap_t;

typedef struct {
    uint32_t bip32PathInt[MAX_BIP32_PATH];
    uint8_t chainCode[32];
} nano_derive_keypair_heap_t;

typedef struct {
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    char buf[128 / 3 + 1 + 2];
    nano_balance_t num;
} nano_format_balance_heap_t;

#endif // NANO_HEAP_H
