/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2016 Ledger
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

#ifndef LIBN_PUBLIC_RAM_VARIABLES_H

#define LIBN_PUBLIC_RAM_VARIABLES_H

#include "blake2b.h"

#include "libn_context.h"
#include "libn_heaps.h"

/** Buffer used for asynchronous response data **/
extern uint8_t libn_async_buffer_D[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];
extern libn_context_t libn_context_D;
extern libn_coin_conf_t libn_coin_conf_D;

/* Different APDU heaps are mutually exclusive,
   so can be mapped to a shared memory space. */
typedef union {
    libn_apdu_get_address_heap_t libn_apdu_get_address_heap_D;
    libn_apdu_cache_block_heap_t libn_apdu_cache_block_heap_D;
    libn_apdu_sign_block_heap_t libn_apdu_sign_block_heap_D;
    libn_apdu_sign_nonce_heap_t libn_apdu_sign_nonce_heap_D;
} ram_a_t;
extern ram_a_t ram_a;

/* This memory mapping has data that is accessed for only a
   short while and never at the same time. */
typedef union {
    blake2b_ctx blake2b_ctx_D;
    libn_derive_keypair_heap_t libn_derive_keypair_heap_D;
    libn_amount_format_heap_t libn_amount_format_heap_D;
} ram_b_t;
extern ram_b_t ram_b;

#endif // LIBN_PUBLIC_RAM_VARIABLES_H
