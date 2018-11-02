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

#ifndef LIBN_HEAPS_H

#define LIBN_HEAPS_H

#include "libn_types.h"
#include "libn_apdu_get_address.h"
#include "libn_apdu_cache_block.h"
#include "libn_apdu_sign_block.h"
#include "libn_apdu_sign_nonce.h"

typedef struct {
    libn_apdu_get_address_request_t req;
    libn_private_key_t privateKey;
} libn_apdu_get_address_heap_t;

typedef struct {
    libn_apdu_cache_block_request_t req;
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    libn_private_key_t privateKey;
} libn_apdu_cache_block_heap_t;

typedef struct {
    libn_private_key_t privateKey;
    libn_block_data_t block;
} libn_apdu_sign_block_heap_input_t;

typedef struct {
    libn_private_key_t privateKey;
    libn_signature_t signature;
} libn_apdu_sign_block_heap_output_t;

typedef struct {
    libn_apdu_sign_block_request_t req;
    union {
        libn_apdu_sign_block_heap_input_t input;
        libn_apdu_sign_block_heap_output_t output;
    } io;
} libn_apdu_sign_block_heap_t;

typedef struct {
    libn_private_key_t privateKey;
    libn_public_key_t publicKey;
    libn_signature_t signature;
} libn_apdu_sign_nonce_heap_output_t;

typedef struct {
    libn_apdu_sign_nonce_request_t req;
    union {
        libn_apdu_sign_nonce_heap_output_t output;
    } io;
} libn_apdu_sign_nonce_heap_t;

typedef struct {
    uint32_t bip32PathInt[MAX_BIP32_PATH];
    uint8_t chainCode[32];
} libn_derive_keypair_heap_t;

typedef struct {
    // MaxUInt128 = 340282366920938463463374607431768211455 (39 digits)
    char buf[39 /* digits */
             + 1 /* period */
             + 1 /* " " before unit */
             + sizeof((libn_coin_conf_t){}.defaultUnit)
             + 1 /* '\0' NULL terminator */];
    libn_amount_t num;
} libn_amount_format_heap_t;

#endif // LIBN_HEAPS_H
