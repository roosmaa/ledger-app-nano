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

#include "os.h"
#include "os_io_seproxyhal.h"

#include "nano_internal.h"
#include "nano_apdu_constants.h"
#include "nano_bagl.h"

#ifdef HAVE_U2F

#include "u2f_service.h"
#include "u2f_transport.h"

void u2f_proxy_response(u2f_service_t *service, uint16_t tx);

#endif

void app_dispatch(void) {
    uint8_t cla;
    uint8_t ins;
    uint8_t dispatched;
    uint16_t statusWord;
    nano_apdu_response_t *resp = &nano_context_D.response;

    // nothing to reply for now
    resp->outLength = 0;
    resp->ioFlags = 0;

    BEGIN_TRY {
        TRY {
            // If halted, then notify
            SB_CHECK(nano_context_D.halted);
            if (SB_GET(nano_context_D.halted)) {
                statusWord = NANO_SW_HALTED;
                goto sendSW;
            }

            cla = G_io_apdu_buffer[ISO_OFFSET_CLA];
            ins = G_io_apdu_buffer[ISO_OFFSET_INS];
            for (dispatched = 0; dispatched < DISPATCHER_APDUS; dispatched++) {
                if ((cla == DISPATCHER_CLA[dispatched]) &&
                    (ins == DISPATCHER_INS[dispatched])) {
                    break;
                }
            }
            if (dispatched == DISPATCHER_APDUS) {
                statusWord = NANO_SW_INS_NOT_SUPPORTED;
                goto sendSW;
            }
            if (DISPATCHER_DATA_IN[dispatched]) {
                if (G_io_apdu_buffer[ISO_OFFSET_LC] == 0x00 ||
                    nano_context_D.inLength - 5 == 0) {
                    statusWord = NANO_SW_INCORRECT_LENGTH;
                    goto sendSW;
                }
                // notify we need to receive data
                // io_exchange(CHANNEL_APDU | IO_RECEIVE_DATA, 0);
            }
            // call the apdu handler
            statusWord = ((apduProcessingFunction)PIC(
                DISPATCHER_FUNCTIONS[dispatched]))(resp);

        sendSW:
            // prepare SW after replied data
            resp->buffer[resp->outLength] = (statusWord >> 8);
            resp->buffer[resp->outLength + 1] = (statusWord & 0xff);
            resp->outLength += 2;
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
            // uncaught exception detected
            resp->outLength = 2;
            resp->buffer[0] = 0x6F;
            resp->buffer[1] = e;
            // we caught something suspicious
            SB_SET(nano_context_D.halted, 1);
        }
        FINALLY;
    }
    END_TRY;
}

void app_async_response(nano_apdu_response_t *resp, uint16_t statusWord) {
    resp->buffer[resp->outLength] = (statusWord >> 8);
    resp->buffer[resp->outLength + 1] = (statusWord & 0xff);
    resp->outLength += 2;

    // Queue up the response to be sent when convenient
    nano_context_D.state = NANO_STATE_READY;
    os_memmove(&nano_context_D.stateData.asyncResponse, resp, sizeof(nano_apdu_response_t));
    app_apply_state();
}

bool app_send_async_response(nano_apdu_response_t *resp) {
#ifdef HAVE_U2F
    if (u2f_activated_D && !nano_context_D.u2fConnected) {
        return false;
    }
#endif

    // Move the async result data to sync buffer
    nano_context_move_async_response();

#ifdef HAVE_U2F
    if (u2f_activated_D) {
        u2f_proxy_response(&u2f_service_D,
            nano_context_D.response.outLength);
    } else {
        io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX,
            nano_context_D.response.outLength);
    }
#else
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX,
        nano_context_D.response.outLength);
#endif
    return true;
}

bool app_apply_state(void) {
    if (!UX_DISPLAYED() || io_seproxyhal_spi_is_status_sent()) {
        return false;
    }

    // First make sure that the UI displays the correct state
    bool uxChanged = nano_bagl_apply_state();
    if (uxChanged) {
        return true;
    }

    // In READY state, try to return the queued asyncResponse
    if (nano_context_D.state == NANO_STATE_READY &&
        nano_context_D.stateData.asyncResponse.outLength > 0) {
        bool responseSent = app_send_async_response(&nano_context_D.stateData.asyncResponse);
        if (responseSent) {
            return true;
        }
    }

    // Everything seems to be in sync
    return false;
}

void app_main(void) {
    os_memset(nano_context_D.response.buffer, 0, 255); // paranoia

    // Process the incoming APDUs

    // first exchange, no out length :) only wait the apdu
    nano_context_D.response.outLength = 0;
    nano_context_D.response.ioFlags = 0;
    for (;;) {
        L_DEBUG_APP(("Main Loop\n"));

        // os_memset(G_io_apdu_buffer, 0, 255); // paranoia

        // receive the whole apdu using the 7 bytes headers (ledger transport)
        nano_context_D.inLength =
            io_exchange(CHANNEL_APDU | nano_context_D.response.ioFlags,
                        // use the previous outlength as the reply
                        nano_context_D.response.outLength);

        app_dispatch();

        // reply during reception of next apdu
    }

    L_DEBUG_APP(("End of main loop\n"));

    // in case reached
    reset();
}
