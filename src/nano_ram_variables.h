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
#include "nano_heap.h"

#ifdef HAVE_U2F
#include "u2f_service.h"
#endif // HAVE_U2F

/** Buffer used for asynchronous response data **/
extern uint8_t nano_async_buffer_D[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];
extern nano_context_t nano_context_D;

/* U2F message buffer and APDU heaps are mutually exclusive,
   so can be mapped to a shared memory space. */
typedef union {
#ifdef HAVE_U2F
    uint8_t u2f_message_buffer[U2F_MAX_MESSAGE_SIZE];
#endif // HAVE_U2F
    nano_apdu_get_address_heap_t nano_apdu_get_address_heap;
    nano_apdu_sign_block_heap_t nano_apdu_sign_block_heap;
} nano_memory_space_a_t;
extern nano_memory_space_a_t nano_memory_space_a_D;

/* This memory mapping has data that is accessed for only a
   short while and never at the same time. */
typedef union {
    blake2b_ctx blake2b_ctx;
    nano_format_balance_heap_t nano_format_balance_heap;
} nano_memory_space_b_t;
extern nano_memory_space_b_t nano_memory_space_b_D;

#ifdef HAVE_U2F
extern u2f_service_t u2f_service_D;
extern bool u2f_activated_D;
#endif // HAVE_U2F

#endif // NANO_PUBLIC_RAM_VARIABLES_H
