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

#ifndef NANO_HELPERS_H

#define NANO_HELPERS_H

#include "nano_types.h"

#define MAX_BIP32_PATH 10
#define MAX_BIP32_PATH_LENGTH (4 * MAX_BIP32_PATH) + 1

bool nano_is_zero(const uint8_t *ptr, size_t num);

uint32_t nano_read_u32(uint8_t *buffer, bool be,
                       bool skipSign);

void nano_write_u32_be(uint8_t *buffer, uint32_t value);
void nano_write_u32_le(uint8_t *buffer, uint32_t value);

void nano_write_hex_string(uint8_t *buffer, const uint8_t *bytes, size_t bytesLen);

bool nano_read_account_string(uint8_t *buffer, size_t size,
                              nano_address_prefix_t *outPrefix,
                              nano_public_key_t outKey);
void nano_write_account_string(uint8_t *buffer, nano_address_prefix_t prefix,
                               const nano_public_key_t publicKey);

int8_t nano_amount_cmp(const nano_amount_t a, const nano_amount_t b);
void nano_amount_subtract(nano_amount_t value, const nano_amount_t other);
void nano_amount_format(char *dest, size_t destLen,
                        const nano_amount_t balance);

void nano_derive_keypair(uint8_t *bip32Path,
                         nano_private_key_t out_privateKey,
                         nano_public_key_t out_publicKey);

/** Implement Java hashCode() equivalent hashing of data **/
uint32_t nano_simple_hash(uint8_t *data, size_t dataLen);

void nano_hash_block(nano_hash_t blockHash,
                     const nano_block_data_t *blockData,
                     const nano_public_key_t publicKey);

void nano_sign_hash(nano_signature_t signature,
                    const nano_hash_t hash,
                    const nano_private_key_t privateKey,
                    const nano_public_key_t publicKey);
bool nano_verify_hash_signature(const nano_hash_t blockHash,
                                const nano_public_key_t publicKey,
                                const nano_signature_t signature);

void nano_sign_nonce(nano_signature_t signature,
                     const nano_nonce_t nonce,
                     const nano_private_key_t privateKey,
                     const nano_public_key_t publicKey);

#endif
