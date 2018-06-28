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

#if defined(TARGET_NANOS)

extern ux_state_t ux;

// display stepped screens
libn_state_t bagl_state;
uint16_t ux_step;
uint16_t ux_step_count;

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
        bool showAmount;
        bool showRecipient;
        bool showRepresentative;
        char confirmLabel[20];
        char confirmValue[MAX(ACCOUNT_BUF_LEN, 2*sizeof(libn_hash_t)+1)];
    } confirmSignBlock;
} vars;

void ui_write_address_truncated(char *label, libn_address_prefix_t prefix, libn_public_key_t publicKey) {
    const size_t addressLen = libn_write_account_string((uint8_t *)label, prefix, publicKey);
    const size_t prefixLen = addressLen - LIBN_ACCOUNT_STRING_BASE_LEN;

    os_memset(label + prefixLen + 5, '.', 2);
    os_memmove(label + prefixLen + 7, label + addressLen - 5, 5);
    label[prefixLen+12] = '\0';
}

void ui_write_address_full(char *label, libn_address_prefix_t prefix, libn_public_key_t publicKey) {
    const size_t addressLen = libn_write_account_string((uint8_t *)label, prefix, publicKey);
    label[addressLen] = '\0';
}

void ui_write_hash_truncated(char *label, libn_hash_t hash) {
    libn_write_hex_string((uint8_t *)label, hash, sizeof(libn_hash_t));
    // Truncate hash to 12345..67890 format
    os_memset(label+5, '.', 2);
    os_memmove(label+7, label+2*sizeof(libn_hash_t)-5, 5);
    label[12] = '\0';
}


const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_autoreceive[];

void menu_settings_autoreceive_change(uint32_t enabled) {
    libn_set_auto_receive(enabled);
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

void menu_settings_autoreceive_init(uint32_t ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_libn.autoReceive ? 1 : 0,
                    menu_settings_autoreceive, NULL);
}

const ux_menu_entry_t menu_settings_autoreceive[] = {
    {NULL, menu_settings_autoreceive_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_autoreceive_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_autoreceive_init, 0, NULL, "Auto-receive", NULL, 0, 0},
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0xAB, NULL, "Version", APPVERSION, 0, 0},
    {NULL, NULL, 0xAB, NULL, "Developer", "Mart Roosmaa", 0, 0},
    // URL with trailing spaces to avoid render artifacts when scrolling
    {NULL, NULL, 0xAB, NULL, "Source code", " github.com/roosmaa/blue-app-nano ", 0, 0},
    {menu_main, NULL, 2, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0xBA, &C_nanos_badge_nano, "Use wallet to",
     "view accounts", 33, 12},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_nanos_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END};

const bagl_element_t *menu_prepro(const ux_menu_entry_t *menu_entry, bagl_element_t *element) {
    // Customise the about menu appearance
    if (menu_entry->userid == 0xAB) {
        switch (element->component.userid) {
        case 0x21: // 1st line
            element->component.font_id = BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER;
            break;
        case 0x22: // 2nd line
            element->component.stroke = 10; // scrolldelay
            element->component.icon_id = 26; // scrollspeed
            break;
        }
    } else if (menu_entry->userid == 0xBA) {
        // Customise the badge for the coin
        switch (element->component.userid) {
        case 0x10: // icon
            element->text = (const char *)COIN_BADGE;
            break;
        }
    }
    return element;
}

void ui_idle(void) {
    bagl_state = LIBN_STATE_READY;
    ux_step_count = 0;
    UX_MENU_DISPLAY(0, menu_main, menu_prepro);
}

void ui_ticker_event(bool uxAllowed) {
    // don't redisplay if UX not allowed (pin locked in the common bolos
    // ux ?)
    if (ux_step_count > 0 && uxAllowed) {
        // prepare next screen
        ux_step = (ux_step + 1) % ux_step_count;
        // redisplay screen
        UX_REDISPLAY();
    }
}

/***
 * Display address
 */

const bagl_element_t ui_display_address[] = {
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 0, /* width */ 128, /* height */ 32,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ 0x000000, /* bgcolor */ 0xFFFFFF,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_ICON, /* userid */ 0x00,
      /* x */ 3, /* y */ 12, /* width */ 7, /* height */ 7,
      /* stroke */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ 0, /* icon_id */ BAGL_GLYPH_ICON_CROSS},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_ICON, /* userid */ 0x00,
      /* x */ 117, /* y */ 13, /* width */ 8, /* height */ 6,
      /* stroke */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ 0, /* icon_id */ BAGL_GLYPH_ICON_CHECK},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x01,
      /* x */ 0, /* y */ 12, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Confirm", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x01,
      /* x */ 0, /* y */ 26, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "address", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x02,
      /* x */ 0, /* y */ 12, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Address", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x02,
      /* x */ 23, /* y */ 26, /* width */ 82, /* height */ 12,
      /* scrolldelay */ 10 | BAGL_STROKE_FLAG_ONESHOT,
      /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 26},
     /* text */ vars.displayAddress.account, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

const bagl_element_t *ui_display_address_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        bool display = (ux_step == element->component.userid - 1);
        if (!display) {
            return NULL;
        }

        switch (element->component.userid) {
        case 1:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case 2:
            UX_CALLBACK_SET_INTERVAL(MAX(
                3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
        }
    }
    return element;
}

uint32_t ui_display_address_button(uint32_t button_mask,
                                   uint32_t button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        libn_bagl_display_address_callback(false);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        libn_bagl_display_address_callback(true);
        break;

    // For other button combinations return early and do nothing
    default:
        return 0;
    }

    ui_idle();
    return 0;
}

void libn_bagl_display_address(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_ADDRESS) {
        return;
    }
    libn_apdu_get_address_request_t *req = &libn_context_D.stateData.getAddressRequest;

    os_memset(&vars.displayAddress, 0, sizeof(vars.displayAddress));
    // Encode public key into an address string
    ui_write_address_full(
      vars.displayAddress.account,
      COIN_DEFAULT_PREFIX,
      req->publicKey);

    bagl_state = LIBN_STATE_CONFIRM_ADDRESS;
    ux_step_count = 2;
    ux_step = 0;
    UX_DISPLAY(ui_display_address, ui_display_address_prepro);
}

/***
 * Confirm sign block
 */

const bagl_element_t ui_confirm_sign_block[] = {
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 0, /* width */ 128, /* height */ 32,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ 0x000000, /* bgcolor */ 0xFFFFFF,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_ICON, /* userid */ 0x00,
      /* x */ 3, /* y */ 12, /* width */ 7, /* height */ 7,
      /* stroke */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ 0, /* icon_id */ BAGL_GLYPH_ICON_CROSS},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_ICON, /* userid */ 0x00,
      /* x */ 117, /* y */ 13, /* width */ 8, /* height */ 6,
      /* stroke */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ 0, /* icon_id */ BAGL_GLYPH_ICON_CHECK},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x01,
      /* x */ 0, /* y */ 12, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Confirm", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x01,
      /* x */ 0, /* y */ 26, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "block", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x02,
      /* x */ 0, /* y */ 12, /* width */ 128, /* height */ 12,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.confirmLabel, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x03,
      /* x */ 23, /* y */ 26, /* width */ 82, /* height */ 12,
      /* scrolldelay */ 10 | BAGL_STROKE_FLAG_ONESHOT,
      /* radius */ 0, /* fill */ 0,
      /* fgcolor */ 0xFFFFFF, /* bgcolor */ 0x000000,
      /* font_id */ BAGL_FONT_OPEN_SANS_EXTRABOLD_11px | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 26},
     /* text */ vars.confirmSignBlock.confirmValue, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

void ui_confirm_sign_block_prepare_confirm_step(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_SIGNATURE) {
        return;
    }
    libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;
    uint8_t step = 1;

    if (ux_step == step++) {
        strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
        ui_write_address_truncated(
            vars.confirmSignBlock.confirmValue,
            COIN_DEFAULT_PREFIX,
            req->publicKey);
        return;
    }

    if (vars.confirmSignBlock.showAmount) {
        if (ux_step == step++) {
            if (vars.confirmSignBlock.showRecipient) {
                strcpy(vars.confirmSignBlock.confirmLabel, "Send amount");
            } else {
                strcpy(vars.confirmSignBlock.confirmLabel, "Receive amount");
            }
            libn_amount_format(
                vars.confirmSignBlock.confirmValue,
                sizeof(vars.confirmSignBlock.confirmValue),
                req->amount);
            return;
        }
    }

    if (vars.confirmSignBlock.showRecipient) {
        if (ux_step == step++) {
            strcpy(vars.confirmSignBlock.confirmLabel, "Send to");
            ui_write_address_full(
                vars.confirmSignBlock.confirmValue,
                req->recipientPrefix,
                req->recipient);
            return;
        }
    }

    if (vars.confirmSignBlock.showRepresentative) {
        if (ux_step == step++) {
            strcpy(vars.confirmSignBlock.confirmLabel, "Representative");
            ui_write_address_full(
                vars.confirmSignBlock.confirmValue,
                req->representativePrefix,
                req->representative);
            return;
        }
    }

    if (ux_step == step++) {
        strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
        ui_write_hash_truncated(
            vars.confirmSignBlock.confirmValue,
            req->blockHash);
        return;
    }
}

const bagl_element_t *ui_confirm_sign_block_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        // Determine which labels are hidden
        if (ux_step == 0) {
            if (element->component.userid != 0x01) {
                return NULL;
            }
        } else {
            if (element->component.userid == 0x01) {
                return NULL;
            }
        }

        // Use a single element (0x02) label to trigger
        // updating the confirm label/value strings.
        if (element->component.userid == 0x02) {
            ui_confirm_sign_block_prepare_confirm_step();
        }

        switch (element->component.userid) {
        case 0x01:
            UX_CALLBACK_SET_INTERVAL(2000);
            break;
        case 0x03:
            UX_CALLBACK_SET_INTERVAL(MAX(
                3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
        }
    }
    return element;
}

uint32_t ui_confirm_sign_block_button(uint32_t button_mask,
                                      uint32_t button_mask_counter) {
    switch (button_mask) {
    case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        libn_bagl_confirm_sign_block_callback(false);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        libn_bagl_confirm_sign_block_callback(true);
        break;

    // For other button combinations return early and do nothing
    default:
        return 0;
    }

    ui_idle();
    return 0;
}

void libn_bagl_confirm_sign_block(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_SIGNATURE) {
        return;
    }
    libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;

    os_memset(&vars.confirmSignBlock, 0, sizeof(vars.confirmSignBlock));

    if (!libn_is_zero(req->amount, sizeof(req->amount))) {
        vars.confirmSignBlock.showAmount = true;

        if (!libn_is_zero(req->recipient, sizeof(req->recipient))) {
            vars.confirmSignBlock.showRecipient = true;
        }
    }
    if (!libn_is_zero(req->representative, sizeof(req->representative))) {
        vars.confirmSignBlock.showRepresentative = true;
    }

    bagl_state = LIBN_STATE_CONFIRM_SIGNATURE;
    ux_step = 0;
    ux_step_count = 3
        + (vars.confirmSignBlock.showAmount ? 1 : 0)
        + (vars.confirmSignBlock.showRecipient ? 1 : 0)
        + (vars.confirmSignBlock.showRepresentative ? 1 : 0);
    UX_DISPLAY(ui_confirm_sign_block, ui_confirm_sign_block_prepro);
}

bool libn_bagl_apply_state() {
    if (!UX_DISPLAYED()) {
        return false;
    }

    switch (libn_context_D.state) {
    case LIBN_STATE_READY:
        if (bagl_state != LIBN_STATE_READY) {
            ui_idle();
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
