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

#include <string.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "glyphs.h"
#include "coins.h"
#include "libn_internal.h"
#include "libn_bagl.h"

#if defined(TARGET_NANOX)

// display stepped screens
libn_state_t bagl_state;

void libn_bagl_idle(void) {
    bagl_state = LIBN_STATE_READY;
    // TODO
}

/***
 * Display address
 */
void libn_bagl_display_address(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_ADDRESS) {
        return;
    }

    // TODO
}

/***
 * Confirm sign block
 */
void libn_bagl_confirm_sign_block(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_SIGNATURE) {
        return;
    }
    
    // TODO
}

void ui_ticker_event(bool uxAllowed) {
}

bool libn_bagl_apply_state() {
    if (!UX_DISPLAYED()) {
        return false;
    }

    switch (libn_context_D.state) {
    case LIBN_STATE_READY:
        if (bagl_state != LIBN_STATE_READY) {
            libn_bagl_idle();
            return true;
        }
        break;
    case LIBN_STATE_CONFIRM_ADDRESS:
        if (bagl_state != LIBN_STATE_CONFIRM_ADDRESS) {
            libn_bagl_display_address();
            return true;
        }
        break;
    case LIBN_STATE_CONFIRM_SIGNATURE:
        if (bagl_state != LIBN_STATE_CONFIRM_SIGNATURE) {
            libn_bagl_confirm_sign_block();
            return true;
        }
        break;
    }

    return false;
}

#endif // defined(TARGET_NANOS)
