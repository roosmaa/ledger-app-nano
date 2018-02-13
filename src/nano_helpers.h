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

uint32_t nano_read_u32(uint8_t *buffer, bool be,
                       bool skipSign);

void nano_write_u32_be(uint8_t *buffer, uint32_t value);
void nano_write_u32_le(uint8_t *buffer, uint32_t value);

void nano_write_hex_string(uint8_t *buffer, uint8_t *bytes, size_t bytesLen);

bool nano_read_account_string(uint8_t *buffer, size_t size,
                              nano_address_prefix_t *outPrefix,
                              nano_public_key_t outKey);
void nano_write_account_string(uint8_t *buffer, nano_address_prefix_t prefix,
                               const nano_public_key_t publicKey);

void nano_truncate_string(char *dest, size_t destLen,
                         char *src, size_t srcLen);

void nano_format_balance(char *dest, size_t destLen,
                         nano_balance_t balance);

void nano_private_derive_keypair(uint8_t *bip32Path,
                                 bool derivePublic);

void nano_hash_block(nano_block_t *block);
void nano_sign_block(nano_block_t *block);

#endif
