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

#ifndef NANO_TYPES_H

#define NANO_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    /** IO flags to reply with at the end of an APDU handler */
    uint8_t ioFlags;
    /** Length of the outgoing command */
    uint16_t outLength;
    /** Pointer to the output buffer for the response */
    uint8_t *buffer;
} nano_apdu_response_t;

typedef enum {
    NANO_STATE_READY,
    NANO_STATE_CONFIRM_ADDRESS,
    NANO_STATE_CONFIRM_SIGNATURE,
} nano_state_t;

typedef uint8_t nano_private_key_t[32];
typedef uint8_t nano_public_key_t[32];
typedef uint8_t nano_hash_t[32];
typedef uint8_t nano_signature_t[64];
typedef uint8_t nano_balance_t[16];

typedef enum {
    NANO_UNKNOWN_BLOCK,
    NANO_OPEN_BLOCK,
    NANO_RECEIVE_BLOCK,
    NANO_SEND_BLOCK,
    NANO_CHANGE_BLOCK,
} nano_block_type_t;

#define NANO_PREFIX_MAX_LEN 5
#define NANO_DEFAULT_PREFIX_LEN 5 // len("nano_")
#define NANO_XRB_PREFIX_LEN 4 // len("xrb_")

typedef enum {
    NANO_DEFAULT_PREFIX, // nano_
    NANO_XRB_PREFIX, // xrb_
} nano_address_prefix_t;

#define NANO_BLOCK_COMMON \
    nano_block_type_t type; \
    nano_hash_t hash; \
    nano_signature_t signature

typedef struct {
    NANO_BLOCK_COMMON;
} nano_block_base_t;

typedef struct {
    NANO_BLOCK_COMMON;
    nano_public_key_t representative;
    nano_address_prefix_t representativePrefix;
    nano_hash_t sourceBlock;
} nano_block_open_t;

typedef struct {
    NANO_BLOCK_COMMON;
    nano_hash_t previousBlock;
    nano_hash_t sourceBlock;
} nano_block_receive_t;

typedef struct {
    NANO_BLOCK_COMMON;
    nano_hash_t previousBlock;
    nano_public_key_t destinationAccount;
    nano_address_prefix_t destinationAccountPrefix;
    nano_balance_t balance;
} nano_block_send_t;

typedef struct {
    NANO_BLOCK_COMMON;
    nano_hash_t previousBlock;
    nano_public_key_t representative;
    nano_address_prefix_t representativePrefix;
} nano_block_change_t;

#undef NANO_BLOCK_COMMON

typedef union {
    nano_block_base_t base;
    nano_block_open_t open;
    nano_block_receive_t receive;
    nano_block_send_t send;
    nano_block_change_t change;
} nano_block_t;

#endif // NANO_TYPES_H