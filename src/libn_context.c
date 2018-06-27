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

#include "libn_internal.h"

/**
 * Initialize the application context on boot
 */
void libn_context_init() {
    L_DEBUG_APP(("Context init\n"));
    os_memset(&libn_context_D, 0, sizeof(libn_context_D));
    SB_SET(libn_context_D.halted, 0);
    libn_context_D.response.buffer = G_io_apdu_buffer;
}

void libn_context_move_async_response(void) {
    libn_apdu_response_t *resp = &libn_context_D.stateData.asyncResponse;

    if (libn_context_D.state != LIBN_STATE_READY) {
        return;
    }
    if (resp->outLength == 0) {
        return;
    }

    // Move the async result data to sync buffer
    libn_context_D.response.outLength = resp->outLength;
    libn_context_D.response.ioFlags = resp->ioFlags;
    os_memmove(libn_context_D.response.buffer, resp->buffer, resp->outLength);

    // Reset the asyncResponse
    resp->ioFlags = 0;
    resp->outLength = 0;
}