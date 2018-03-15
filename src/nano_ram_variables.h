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

#ifndef NANO_PUBLIC_RAM_VARIABLES_H

#define NANO_PUBLIC_RAM_VARIABLES_H

#include "blake2b.h"

#include "nano_context.h"
#include "nano_heaps.h"

/** Buffer used for asynchronous response data **/
extern uint8_t nano_async_buffer_D[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];
extern nano_context_t nano_context_D;

/* Different APDU heaps are mutually exclusive,
   so can be mapped to a shared memory space. */
typedef union {
    nano_apdu_get_address_heap_t nano_apdu_get_address_heap_D;
    nano_apdu_validate_block_heap_t nano_apdu_validate_block_heap_D;
    nano_apdu_sign_block_heap_t nano_apdu_sign_block_heap_D;
} ram_a_t;
extern ram_a_t ram_a;

/* This memory mapping has data that is accessed for only a
   short while and never at the same time. */
typedef union {
    blake2b_ctx blake2b_ctx_D;
    nano_derive_keypair_heap_t nano_derive_keypair_heap_D;
    nano_amount_format_heap_t nano_amount_format_heap_D;
} ram_b_t;
extern ram_b_t ram_b;

#endif // NANO_PUBLIC_RAM_VARIABLES_H
