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

#define ACCOUNT_BUF_LEN ( \
    LIBN_ACCOUNT_STRING_BASE_LEN \
    + MAX(sizeof(COIN_PRIMARY_PREFIX), \
          sizeof(COIN_SECONDARY_PREFIX)) \
    + 1 \
)
union {
    struct {
        char account[ACCOUNT_BUF_LEN];
    } displayAddress;
    struct {
        bool showRecipient;
        char titleBuf[20];
        char textBuf[MAX(ACCOUNT_BUF_LEN, 2*sizeof(libn_hash_t)+1)];
    } confirmSignBlock;
} vars;

libn_state_t bagl_state;

void ui_write_address_truncated(const libn_address_formatter_t *fmt,
                                char *label,
                                const libn_public_key_t publicKey) {
    const size_t addressLen = libn_address_format(fmt, (uint8_t *)label, publicKey);
    const size_t prefixLen = addressLen - LIBN_ACCOUNT_STRING_BASE_LEN;

    os_memset(label + prefixLen + 5, '.', 2);
    os_memmove(label + prefixLen + 7, label + addressLen - 5, 5);
    label[prefixLen+12] = '\0';
}

void ui_write_address_full(const libn_address_formatter_t *fmt,
                           char *label,
                           const libn_public_key_t publicKey) {
    const size_t addressLen = libn_address_format(fmt, (uint8_t *)label, publicKey);
    label[addressLen] = '\0';
}

void ui_write_hash_truncated(char *label, libn_hash_t hash) {
    libn_write_hex_string((uint8_t *)label, hash, sizeof(libn_hash_t));
    // Truncate hash to 12345..67890 format
    os_memset(label+5, '.', 2);
    os_memmove(label+7, label+2*sizeof(libn_hash_t)-5, 5);
    label[12] = '\0';
}

/***
 * Idle screen
 */
const char *settings_auto_receive_getter(unsigned int idx);
void settings_auto_receive_selector(unsigned int idx);
const char *settings_submenu_getter(unsigned int idx);
void settings_submenu_selector(unsigned int idx);
const ux_flow_step_t * const ux_about_flow[];

UX_STEP_NOCB(
    ux_idle_flow_1_step,
    nn,
    {
      "Application",
      "is ready",
    });
UX_STEP_CB(
    ux_idle_flow_2_step,
    pb,
    ux_menulist_init(0, settings_submenu_getter, settings_submenu_selector),
    {
      &C_icon_coggle,
      "Settings",
    });
UX_STEP_CB(
    ux_idle_flow_3_step,
    bn,
    ux_flow_init(0, ux_about_flow, NULL),
    {
      "Version",
      APPVERSION,
    });
UX_STEP_CB(
    ux_idle_flow_4_step,
    pb,
    os_sched_exit(-1),
    {
      &C_icon_dashboard,
      "Quit",
    });
UX_FLOW(ux_idle_flow,
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step,
    &ux_idle_flow_4_step);

const char * const settings_auto_receive_getter_values[] = {
    "Disabled",
    "Enabled",
    "Back"
};

const char *settings_auto_receive_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_auto_receive_getter_values)) {
        return settings_auto_receive_getter_values[idx];
    }
    return NULL;
}

void settings_auto_receive_selector(unsigned int idx) {
    switch(idx) {
    case 0:
        libn_set_auto_receive(false);
        break;
    case 1:
        libn_set_auto_receive(true);
        break;
    default:
        break;
    }
    ux_menulist_init_select(0, settings_submenu_getter, settings_submenu_selector, 0);
}

const char * const settings_submenu_getter_values[] = {
    "Auto-receive",
    "Back",
};

const char *settings_submenu_getter(unsigned int idx) {
    if (idx < ARRAYLEN(settings_submenu_getter_values)) {
        return settings_submenu_getter_values[idx];
    }
    return NULL;
}

void settings_submenu_selector(unsigned int idx) {
    switch(idx) {
    case 0:
        ux_menulist_init_select(0, settings_auto_receive_getter, settings_auto_receive_selector, N_libn.autoReceive ? 1 : 0);
        break;
    default:
        ux_flow_init(0, ux_idle_flow, &ux_idle_flow_2_step);
    }
}

UX_STEP_CB(
    ux_about_flow_1_step,
    nn,
    ux_flow_init(0, ux_idle_flow, &ux_idle_flow_3_step),
    {
      "About",
      "application",
    });
UX_STEP_CB(
    ux_about_flow_2_step,
    bn,
    ux_flow_init(0, ux_idle_flow, &ux_idle_flow_3_step),
    {
      "Developer",
      "Mart Roosmaa",
    });
UX_STEP_CB(
    ux_about_flow_3_step,
    bnnn,
    ux_flow_init(0, ux_idle_flow, &ux_idle_flow_3_step),
    {
      "Source code",
      "github.com/",
      "roosmaa/",
      "blue-app-nano",
    });
UX_STEP_CB(
    ux_about_flow_4_step,
    pb,
    ux_flow_init(0, ux_idle_flow, &ux_idle_flow_3_step),
    {
      &C_icon_close,
      "Back",
    });
UX_FLOW(ux_about_flow,
    &ux_about_flow_1_step,
    &ux_about_flow_2_step,
    &ux_about_flow_3_step,
    &ux_about_flow_4_step);

void libn_bagl_idle(void) {
    bagl_state = LIBN_STATE_READY;
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

/***
 * Display address
 */
UX_STEP_NOCB(
    ux_display_address_flow_1_step, 
    pnn, 
    {
        &C_icon_eye,
        "Confirm",
        "address",
    });
UX_STEP_NOCB(
    ux_display_address_flow_2_step,
    bnnn_paging,
    {
        "Address",
        vars.displayAddress.account,
    });
UX_STEP_CB(
    ux_display_address_flow_3_step,
    pb,
    libn_bagl_display_address_callback(true),
    {
        &C_icon_validate_14,
        "Confirm",
    });
UX_STEP_CB(
    ux_display_address_flow_4_step,
    pb,
    libn_bagl_display_address_callback(false),
    {
        &C_icon_crossmark,
        "Reject",
    });
UX_FLOW(ux_display_address_flow,
    &ux_display_address_flow_1_step,
    &ux_display_address_flow_2_step,
    &ux_display_address_flow_3_step,
    &ux_display_address_flow_4_step);

void libn_bagl_display_address(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_ADDRESS) {
        return;
    }
    libn_apdu_get_address_request_t *req = &libn_context_D.stateData.getAddressRequest;

    os_memset(&vars.displayAddress, 0, sizeof(vars.displayAddress));
    // Encode public key into an address string
    ui_write_address_full(
        &req->addressFormatter,
        vars.displayAddress.account,
        req->publicKey);

    bagl_state = LIBN_STATE_CONFIRM_ADDRESS;
    ux_flow_init(0, ux_display_address_flow, NULL);
}

/***
 * Confirm sign block
 */
UX_STEP_NOCB(
    ux_confirm_sign_block_flow_1_step, 
    pnn, 
    {
        &C_icon_eye,
        "Review",
        "transaction",
    });
UX_STEP_NOCB_INIT(
    ux_confirm_sign_block_flow_2_step,
    bn,
    {
        libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
        ui_write_address_truncated(
            &req->addressFormatter,
            vars.confirmSignBlock.textBuf,
            req->publicKey);
    },
    {
        "Your account",
        vars.confirmSignBlock.textBuf,
    });
UX_STEP_NOCB_INIT(
    ux_confirm_sign_block_flow_3_step,
    bn,
    {
        libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
        if (vars.confirmSignBlock.showRecipient) {
            strcpy(vars.confirmSignBlock.titleBuf, "Send amount");
        } else {
            strcpy(vars.confirmSignBlock.titleBuf, "Receive amount");
        }
        libn_amount_format(
            &req->amountFormatter,
            vars.confirmSignBlock.textBuf,
            sizeof(vars.confirmSignBlock.textBuf),
            req->amount);
    },
    {
        vars.confirmSignBlock.titleBuf,
        vars.confirmSignBlock.textBuf,
    });
UX_STEP_NOCB_INIT(
    ux_confirm_sign_block_flow_4_step,
    bnnn_paging,
    {
        libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
        ui_write_address_full(
            &req->recipientFormatter,
            vars.confirmSignBlock.textBuf,
            req->recipient);
    },
    {
        "Send to",
        vars.confirmSignBlock.textBuf,
    });
UX_STEP_NOCB_INIT(
    ux_confirm_sign_block_flow_5_step,
    bnnn_paging,
    {
        libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
        ui_write_address_full(
            &req->representativeFormatter,
            vars.confirmSignBlock.textBuf,
            req->representative);
    },
    {
        "Representative",
        vars.confirmSignBlock.textBuf,
    });
UX_STEP_NOCB_INIT(
    ux_confirm_sign_block_flow_6_step,
    bn,
    {
        libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
        ui_write_hash_truncated(
            vars.confirmSignBlock.textBuf,
            req->blockHash);
    },
    {
        "Block hash",
        vars.confirmSignBlock.textBuf,
    });
UX_STEP_CB(
    ux_confirm_sign_block_flow_7_step,
    pbb,
    libn_bagl_confirm_sign_block_callback(true),
    {
        &C_icon_validate_14,
        "Accept",
    });
UX_STEP_CB(
    ux_confirm_sign_block_flow_8_step,
    pb,
    libn_bagl_confirm_sign_block_callback(false),
    {
        &C_icon_crossmark,
        "Reject",
    });
const ux_flow_step_t * ux_confirm_sign_block_flow[9];

void libn_bagl_confirm_sign_block(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_SIGNATURE) {
        return;
    }
    libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;

    os_memset(&vars.confirmSignBlock, 0, sizeof(vars.confirmSignBlock));

    bool showAmount = false;
    bool showRecipient = false;
    bool showRepresentative = false;

    if (!libn_is_zero(req->amount, sizeof(req->amount))) {
        showAmount = true;

        if (!libn_is_zero(req->recipient, sizeof(req->recipient))) {
            showRecipient = true;
        }
    }
    if (!libn_is_zero(req->representative, sizeof(req->representative))) {
        showRepresentative = true;
    }

    vars.confirmSignBlock.showRecipient = showRecipient;
    bagl_state = LIBN_STATE_CONFIRM_SIGNATURE;

    int step = 0;
    ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_1_step;
    ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_2_step;
    if (showAmount) ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_3_step;
    if (showRecipient) ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_4_step;
    if (showRepresentative) ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_5_step;
    ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_6_step;
    ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_7_step;
    ux_confirm_sign_block_flow[step++] = &ux_confirm_sign_block_flow_8_step;
    ux_confirm_sign_block_flow[step++] = FLOW_END_STEP;

    ux_flow_init(0, ux_confirm_sign_block_flow, NULL);
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
