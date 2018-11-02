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

#ifndef LIBN_TYPES_H

#define LIBN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "os_io_seproxyhal.h"

typedef struct {
    /** IO flags to reply with at the end of an APDU handler */
    uint8_t ioFlags;
    /** Length of the outgoing command */
    uint16_t outLength;
    /** Pointer to the output buffer for the response */
    uint8_t *buffer;
} libn_apdu_response_t;

typedef enum {
    LIBN_STATE_READY,
    LIBN_STATE_CONFIRM_ADDRESS,
    LIBN_STATE_CONFIRM_SIGNATURE,
} libn_state_t;

typedef uint8_t libn_private_key_t[32];
typedef uint8_t libn_public_key_t[32];
typedef uint8_t libn_hash_t[32];
typedef uint8_t libn_link_t[32];
typedef uint8_t libn_signature_t[64];
typedef uint8_t libn_amount_t[16];
typedef uint8_t libn_nonce_t[16];

typedef enum {
    LIBN_COIN_TYPE_NANO,
    LIBN_COIN_TYPE_BANANO,
    LIBN_COIN_TYPE_NOS,
} libn_coin_type_t;

typedef enum {
    LIBN_PRIMARY_PREFIX,
    LIBN_SECONDARY_PREFIX,
} libn_address_prefix_t;

typedef struct {
    const char coinName[10];
    const bagl_icon_details_t *coinBadge;
    const uint32_t bip32Prefix[2];
    const char addressPrimaryPrefix[6];
    const char addressSecondaryPrefix[6];
    const libn_address_prefix_t addressDefaultPrefix;
    const char defaultUnit[10];
    const uint8_t defaultUnitScale;
#if defined(TARGET_BLUE)
    const uint32_t colorBackground;
    const uint32_t colorForeground;
    const uint32_t colorAltBackground;
    const uint32_t colorAltForeground;
    const uint32_t colorRejectBackground;
    const uint32_t colorRejectForeground;
    const uint32_t colorRejectOverBackground;
    const uint32_t colorRejectOverForeground;
    const uint32_t colorConfirmBackground;
    const uint32_t colorConfirmForeground;
    const uint32_t colorConfirmOverBackground;
    const uint32_t colorConfirmOverForeground;
    const bagl_icon_details_t *iconToggleOff;
    const bagl_icon_details_t *iconToggleOn;
#endif // TARGET_BLUE
} libn_coin_conf_t;

typedef struct {
    char *prefix;
    uint8_t prefixLen;
} libn_address_formatter_t;

typedef struct {
    char *suffix;
    uint8_t suffixLen;
    uint8_t unitScale;
} libn_amount_formatter_t;

typedef struct {
    libn_hash_t parent;
    libn_link_t link;
    libn_public_key_t representative;
    libn_amount_t balance;
} libn_block_data_t;

typedef struct {
    libn_hash_t hash;
    libn_public_key_t representative;
    libn_amount_t balance;
} libn_cached_block_data_t;

#endif // LIBN_TYPES_H