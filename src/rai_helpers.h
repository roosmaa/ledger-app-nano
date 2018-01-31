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

#ifndef RAI_HELPERS_H

#define RAI_HELPERS_H

#include "rai_types.h"

#define MAX_BIP32_PATH 10
#define MAX_BIP32_PATH_LENGTH (4 * MAX_BIP32_PATH) + 1

uint32_t rai_read_u32(uint8_t *buffer, bool be,
                      bool skipSign);

void rai_write_u32_be(uint8_t *buffer, uint32_t value);
void rai_write_u32_le(uint8_t *buffer, uint32_t value);

void rai_write_hex_string(uint8_t *buffer, uint8_t *bytes, size_t bytesLen);

bool rai_read_account_string(uint8_t *buffer, size_t size,
                             rai_address_prefix_t *outPrefix,
                             rai_public_key_t outKey);
void rai_write_account_string(uint8_t *buffer, rai_address_prefix_t prefix,
                              const rai_public_key_t publicKey);

void rai_truncate_string(char *dest, size_t destLen,
                         char *src, size_t srcLen);

void rai_format_balance(char *dest, size_t destLen,
                        rai_balance_t balance);

void rai_private_derive_keypair(uint8_t *bip32Path,
                                bool derivePublic,
                                uint8_t *out_chainCode);

void rai_hash_block(rai_block_t *block);
void rai_sign_block(rai_block_t *block);

#endif
