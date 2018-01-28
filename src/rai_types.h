/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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

#ifndef RAI_TYPES_H

#define RAI_TYPES_H

#include <stdint.h>

typedef uint8_t rai_private_key_t[32];
typedef uint8_t rai_public_key_t[32];
typedef uint8_t rai_hash_t[32];
typedef uint8_t rai_signature_t[64];
typedef uint8_t rai_balance_t[16];

typedef enum {
    RAI_UNKNOWN_BLOCK,
    RAI_OPEN_BLOCK,
    RAI_RECEIVE_BLOCK,
    RAI_SEND_BLOCK,
    RAI_CHANGE_BLOCK,
} block_type_t;

#define RAI_BLOCK_COMMON \
    block_type_t type; \
    rai_hash_t hash; \
    rai_signature_t signature

typedef struct {
    RAI_BLOCK_COMMON;
} rai_block_base_t;

typedef struct {
    RAI_BLOCK_COMMON;
    rai_public_key_t representative;
    rai_hash_t sourceBlock;
} rai_block_open_t;

typedef struct {
    RAI_BLOCK_COMMON;
    rai_hash_t previousBlock;
    rai_hash_t sourceBlock;
} rai_block_receive_t;

typedef struct {
    RAI_BLOCK_COMMON;
    rai_hash_t previousBlock;
    rai_public_key_t destinationAccount;
    rai_balance_t balance;
} rai_block_send_t;

typedef struct {
    RAI_BLOCK_COMMON;
    rai_hash_t previousBlock;
    rai_public_key_t representative;
} rai_block_change_t;

#undef RAI_BLOCK_COMMON

typedef union {
    rai_block_base_t base;
    rai_block_open_t open;
    rai_block_receive_t receive;
    rai_block_send_t send;
    rai_block_change_t change;
} rai_block_t;

#endif // RAI_TYPES_H