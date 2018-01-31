/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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
#include "rai_internal.h"
#include "rai_bagl.h"

#if defined(TARGET_NANOS)

extern void USB_power_U2F(bool enabled, bool fido);
extern ux_state_t ux;

// display stepped screens
uint16_t ux_step;
uint16_t ux_step_count;

union {
    struct {
        char account[RAI_ACCOUNT_STRING_BASE_LEN+RAI_PREFIX_MAX_LEN+1];
    } displayAddress;
    struct {
        char blockType[20];
        char confirmLabel[20];
        char confirmValue[RAI_ACCOUNT_STRING_BASE_LEN+RAI_PREFIX_MAX_LEN+1];
    } confirmSignBlock;
} vars;

void ui_write_address_truncated(char *label, rai_address_prefix_t prefix, rai_public_key_t publicKey) {
    char buf[RAI_ACCOUNT_STRING_BASE_LEN+RAI_PREFIX_MAX_LEN];
    rai_write_account_string((uint8_t *)buf, prefix, publicKey);

    size_t addressSize;
    switch (prefix) {
    case RAI_DEFAULT_PREFIX:
      addressSize = RAI_ACCOUNT_STRING_BASE_LEN + RAI_DEFAULT_PREFIX_LEN;
      break;
    case RAI_XRB_PREFIX:
      addressSize = RAI_ACCOUNT_STRING_BASE_LEN + RAI_XRB_PREFIX_LEN;
      break;
    }
    rai_truncate_string(label, 13, buf, addressSize);
}

void ui_write_address_full(char *label, rai_address_prefix_t prefix, rai_public_key_t publicKey) {
    rai_write_account_string((uint8_t *)label, prefix, publicKey);
    switch (prefix) {
    case RAI_DEFAULT_PREFIX:
      label[RAI_ACCOUNT_STRING_BASE_LEN+RAI_DEFAULT_PREFIX_LEN] = '\0';
      break;
    case RAI_XRB_PREFIX:
      label[RAI_ACCOUNT_STRING_BASE_LEN+RAI_XRB_PREFIX_LEN] = '\0';
      break;
    }
}

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_autoreceive[];
const ux_menu_entry_t menu_settings_browser[];

#ifdef HAVE_U2F

// change the setting
void menu_settings_browser_change(uint32_t enabled) {
    rai_set_fido_transport(enabled);
    USB_power_U2F(false, false);
    USB_power_U2F(true, N_rai.fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(1, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(uint32_t ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_rai.fidoTransport ? 1 : 0,
                    menu_settings_browser, NULL);
}

const ux_menu_entry_t menu_settings_browser[] = {
    {NULL, menu_settings_browser_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_browser_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};
#endif // HAVE_U2F

void menu_settings_autoreceive_change(uint32_t enabled) {
    rai_set_auto_receive(enabled);
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

void menu_settings_autoreceive_init(uint32_t ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_rai.autoReceive ? 1 : 0,
                    menu_settings_autoreceive, NULL);
}

const ux_menu_entry_t menu_settings_autoreceive[] = {
    {NULL, menu_settings_autoreceive_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_autoreceive_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_autoreceive_init, 0, NULL, "Auto-receive", NULL, 0, 0},
#ifdef HAVE_U2F
    {NULL, menu_settings_browser_init, 0, NULL, "Browser support", NULL, 0, 0},
#endif // HAVE_U2F
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
    {menu_main, NULL, 2, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_nanos_badge_raiblocks, "Use wallet to",
     "view accounts", 33, 12},
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
    {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
    {NULL, os_sched_exit, 0, &C_nanos_icon_dashboard, "Quit app", NULL, 50, 29},
    UX_MENU_END};

void ui_idle(void) {
    ux_step_count = 0;
    UX_MENU_DISPLAY(0, menu_main, NULL);
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
        rai_bagl_display_address_callback(false);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        rai_bagl_display_address_callback(true);
        break;

    // For other button combinations return early and do nothing
    default:
        return 0;
    }

    ui_idle();
    return 0;
}

void rai_bagl_display_address(void) {
    os_memset(&vars.displayAddress, 0, sizeof(vars.displayAddress));
    // Encode public key into an address string
    ui_write_address_full(
      vars.displayAddress.account,
      RAI_DEFAULT_PREFIX,
      rai_public_key_D);

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
     /* text */ vars.confirmSignBlock.blockType, /* touch_area_brim */ 0,
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

void ui_write_confirm_label_block_hash(char *label, rai_hash_t hash) {
    char buf[2*sizeof(rai_hash_t)];
    rai_write_hex_string((uint8_t *)buf, hash, sizeof(rai_hash_t));
    rai_truncate_string(label, 13, buf, sizeof(buf));
}

void ui_confirm_sign_block_prepare_confirm_step(void) {
    switch (rai_context_D.block.base.type) {
    case RAI_UNKNOWN_BLOCK:
        switch (ux_step) {
        default:
        case 1:
            strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
            ui_write_address_truncated(
                vars.confirmSignBlock.confirmValue,
                RAI_DEFAULT_PREFIX,
                rai_public_key_D);
            break;
        case 2:
            strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
            ui_write_confirm_label_block_hash(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.base.hash);
            break;
        }
        break;
    case RAI_OPEN_BLOCK:
        switch (ux_step) {
        default:
        case 1:
            strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
            ui_write_address_truncated(
                vars.confirmSignBlock.confirmValue,
                RAI_DEFAULT_PREFIX,
                rai_public_key_D);
            break;
        case 2:
            strcpy(vars.confirmSignBlock.confirmLabel, "Represtative");
            ui_write_address_full(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.open.representativePrefix,
                rai_context_D.block.open.representative);
            break;
        case 3:
            strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
            ui_write_confirm_label_block_hash(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.open.hash);
            break;
        }
        break;
    case RAI_RECEIVE_BLOCK:
        switch (ux_step) {
        default:
        case 1:
            strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
            ui_write_address_truncated(
                vars.confirmSignBlock.confirmValue,
                RAI_DEFAULT_PREFIX,
                rai_public_key_D);
            break;
        case 2:
            strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
            ui_write_confirm_label_block_hash(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.receive.hash);
            break;
        }
        break;
    case RAI_SEND_BLOCK:
        switch (ux_step) {
        default:
        case 1:
            strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
            ui_write_address_truncated(
                vars.confirmSignBlock.confirmValue,
                RAI_DEFAULT_PREFIX,
                rai_public_key_D);
            break;
        case 2:
            strcpy(vars.confirmSignBlock.confirmLabel, "Balance after");
            rai_format_balance(
                vars.confirmSignBlock.confirmValue,
                sizeof(vars.confirmSignBlock.confirmValue),
                rai_context_D.block.send.balance);
            break;
        case 3:
            strcpy(vars.confirmSignBlock.confirmLabel, "Send to");
            ui_write_address_full(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.send.destinationAccountPrefix,
                rai_context_D.block.send.destinationAccount);
            break;
        case 4:
            strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
            ui_write_confirm_label_block_hash(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.send.hash);
            break;
        }
        break;
    case RAI_CHANGE_BLOCK:
        switch (ux_step) {
        default:
        case 1:
            strcpy(vars.confirmSignBlock.confirmLabel, "Your account");
            ui_write_address_truncated(
                vars.confirmSignBlock.confirmValue,
                RAI_DEFAULT_PREFIX,
                rai_public_key_D);
            break;
        case 2:
            strcpy(vars.confirmSignBlock.confirmLabel, "Represtative");
            ui_write_address_full(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.change.representativePrefix,
                rai_context_D.block.change.representative);
            break;
        case 3:
            strcpy(vars.confirmSignBlock.confirmLabel, "Block hash");
            ui_write_confirm_label_block_hash(
                vars.confirmSignBlock.confirmValue,
                rai_context_D.block.change.hash);
            break;
        }
        break;
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
        rai_bagl_confirm_sign_block_callback(false);
        break;

    case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
        rai_bagl_confirm_sign_block_callback(true);
        break;

    // For other button combinations return early and do nothing
    default:
        return 0;
    }

    ui_idle();
    return 0;
}

void rai_bagl_confirm_sign_block(void) {
    os_memset(&vars.confirmSignBlock, 0, sizeof(vars.confirmSignBlock));

    switch (rai_context_D.block.base.type) {
    case RAI_UNKNOWN_BLOCK:
        strcpy(vars.confirmSignBlock.blockType, "unknown block");
        ux_step_count = 3;
        break;
    case RAI_OPEN_BLOCK:
        strcpy(vars.confirmSignBlock.blockType, "open block");
        ux_step_count = 4;
        break;
    case RAI_RECEIVE_BLOCK:
        strcpy(vars.confirmSignBlock.blockType, "receive block");
        ux_step_count = 3;
        break;
    case RAI_SEND_BLOCK:
        strcpy(vars.confirmSignBlock.blockType, "send block");
        ux_step_count = 5;
        break;
    case RAI_CHANGE_BLOCK:
        strcpy(vars.confirmSignBlock.blockType, "change block");
        ux_step_count = 4;
        break;
    }

    ux_step = 0;
    UX_DISPLAY(ui_confirm_sign_block, ui_confirm_sign_block_prepro);
}

#endif // defined(TARGET_NANOS)
