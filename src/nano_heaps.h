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

#ifndef NANO_HEAPS_H

#define NANO_HEAPS_H

#include "nano_types.h"
#include "nano_apdu_get_address.h"
#include "nano_apdu_validate_block.h"
#include "nano_apdu_sign_block.h"
#include "nano_apdu_sign_nonce.h"

typedef struct {
    nano_apdu_get_address_request_t req;
    nano_private_key_t privateKey;
} nano_apdu_get_address_heap_t;

typedef struct {
    nano_apdu_validate_block_request_t req;
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    nano_private_key_t privateKey;
} nano_apdu_validate_block_heap_t;

typedef struct {
    nano_private_key_t privateKey;
    nano_block_data_t block;
} nano_apdu_sign_block_heap_input_t;

typedef struct {
    nano_private_key_t privateKey;
    nano_signature_t signature;
} nano_apdu_sign_block_heap_output_t;

typedef struct {
    nano_apdu_sign_block_request_t req;
    union {
        nano_apdu_sign_block_heap_input_t input;
        nano_apdu_sign_block_heap_output_t output;
    } io;
} nano_apdu_sign_block_heap_t;

typedef struct {
    nano_private_key_t privateKey;
    nano_public_key_t publicKey;
    nano_hash_t nonceHash;
    nano_signature_t signature;
} nano_apdu_sign_nonce_heap_output_t;

typedef struct {
    nano_apdu_sign_nonce_request_t req;
    union {
        nano_apdu_sign_nonce_heap_output_t output;
    } io;
} nano_apdu_sign_nonce_heap_t;

typedef struct {
    uint32_t bip32PathInt[MAX_BIP32_PATH];
    uint8_t chainCode[32];
} nano_derive_keypair_heap_t;

typedef struct {
    // MaxUInt128 = 340282366920938463463374607431768211455 (39 digits)
    // 39 digits + 1 period + len(" Nano") + len('\0')
    char buf[39 + 1 + 5 + 1];
    nano_amount_t num;
} nano_amount_format_heap_t;

#endif // NANO_HEAPS_H
