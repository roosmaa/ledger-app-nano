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

#ifndef NANO_CONTEXT_H

#define NANO_CONTEXT_H

#include "os.h"
#include "nano_types.h"
#include "nano_secure_value.h"
#include "nano_apdu_get_address.h"
#include "nano_apdu_sign_block.h"

typedef struct {
    /** Flag if dongle has been halted */
    secu8 halted;

    /** Length of the incoming command */
    uint16_t inLength;

    /** Primary response for synchronous APDUs **/
    nano_apdu_response_t response;

    /** Cached block for determining the deltas for the child block **/
    nano_cached_block_data_t cachedBlock;

    /** State determines the application state (UX displayed, etc).
        This is also used to enforce only single confirmation
        prompt. **/
    nano_state_t state;
    union {
        // when NANO_STATE_READY
        nano_apdu_response_t asyncResponse;
        // when NANO_STATE_CONFIRM_ADDRESS
        nano_apdu_get_address_request_t getAddressRequest;
        // when NANO_STATE_CONFIRM_SIGNATURE
        nano_apdu_sign_block_request_t signBlockRequest;
    } stateData;

#ifdef HAVE_IO_U2F
    /** U2F async request hash that is used to keep track and hook back
        into an async operation. **/
    uint32_t u2fRequestHash;
    /** U2F timeout tracker, once zero the active connection is dropped
        appropriate status code. **/
    uint16_t u2fTimeout;
#endif // HAVE_IO_U2F

} nano_context_t;

void nano_context_init(void);
void nano_context_move_async_response(void);

#endif
