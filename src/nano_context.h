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

    /** Buffer used for asynchronous response data **/
    uint8_t asyncBuffer[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];

    /** State determines the application state (UX displayed, etc).
        This is also used to enforce only single confirmation
        prompt. **/
    nano_state_t state;
    union {
        // when NANO_STATE_READY
        nano_apdu_response_t asyncResponse;
        // when NANO_STATE_CONFIRM_ADDRESS
        nano_apdu_get_address_request getAddressRequest;
        // when NANO_STATE_CONFIRM_SIGNATURE
        nano_apdu_sign_block_request signBlockRequest;
    } stateData;

} nano_context_t;

void nano_context_init(void);

#endif
