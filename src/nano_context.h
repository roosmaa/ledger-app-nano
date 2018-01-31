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

struct nano_context_s {
    /** Flag if dongle has been halted */
    secu8 halted;

    /** Chain code for the current public key **/
    uint8_t chainCode[32];

    /** Currently processed block **/
    nano_block_t block;

    /** Length of the incoming command */
    uint16_t inLength;
    /** Length of the outgoing command */
    uint16_t outLength;

    /** IO flags to reply with at the end of an APDU handler */
    uint8_t ioFlags;

    /** Status Word of the response */
    uint16_t sw;
};
typedef struct nano_context_s nano_context_t;

void nano_context_init(void);

#endif
