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

#ifndef LIBN_HELPERS_H

#define LIBN_HELPERS_H

#include "libn_types.h"

#define MAX_BIP32_PATH 10
#define MAX_BIP32_PATH_LENGTH (4 * MAX_BIP32_PATH) + 1

bool libn_is_zero(const uint8_t *ptr, size_t num);

uint32_t libn_read_u32(uint8_t *buffer, bool be,
                       bool skipSign);

void libn_write_u32_be(uint8_t *buffer, uint32_t value);
void libn_write_u32_le(uint8_t *buffer, uint32_t value);

void libn_write_hex_string(uint8_t *buffer, const uint8_t *bytes, size_t bytesLen);

void libn_address_formatter_for_coin(libn_address_formatter_t *fmt,
                                     libn_address_prefix_t prefix,
                                     uint8_t *bip32Path);
void libn_amount_formatter_for_coin(libn_amount_formatter_t *fmt,
                                    uint8_t *bip32Path);

size_t libn_address_format(const libn_address_formatter_t *fmt,
                           uint8_t *buffer,
                           const libn_public_key_t publicKey);

int8_t libn_amount_cmp(const libn_amount_t a, const libn_amount_t b);
void libn_amount_subtract(libn_amount_t value, const libn_amount_t other);
void libn_amount_format(const libn_amount_formatter_t *fmt,
                        char *dest, size_t destLen,
                        const libn_amount_t balance);

void libn_derive_keypair(uint8_t *bip32Path,
                         libn_private_key_t out_privateKey,
                         libn_public_key_t out_publicKey);

/** Implement Java hashCode() equivalent hashing of data **/
uint32_t libn_simple_hash(uint8_t *data, size_t dataLen);

void libn_hash_block(libn_hash_t blockHash,
                     const libn_block_data_t *blockData,
                     const libn_public_key_t publicKey);

void libn_sign_hash(libn_signature_t signature,
                    const libn_hash_t hash,
                    const libn_private_key_t privateKey,
                    const libn_public_key_t publicKey);
bool libn_verify_hash_signature(const libn_hash_t blockHash,
                                const libn_public_key_t publicKey,
                                const libn_signature_t signature);

void libn_sign_nonce(libn_signature_t signature,
                     const libn_nonce_t nonce,
                     const libn_private_key_t privateKey,
                     const libn_public_key_t publicKey);

#endif
