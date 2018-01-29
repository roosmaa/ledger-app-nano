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
        char account[ACCOUNT_STRING_LEN+1];
    } display_address;
} vars;

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_browser[];

#ifdef HAVE_U2F

// change the setting
void menu_settings_browser_change(uint32_t enabled) {
    rai_set_fido_transport(enabled);
    USB_power_U2F(false, false);
    USB_power_U2F(true, N_rai.fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(uint32_t ignored) {
    UNUSED(ignored);
    UX_MENU_DISPLAY(N_rai.fidoTransport ? 1 : 0, menu_settings_browser,
                    NULL);
}

const ux_menu_entry_t menu_settings_browser[] = {
    {NULL, menu_settings_browser_change, 0, NULL, "No", NULL, 0, 0},
    {NULL, menu_settings_browser_change, 1, NULL, "Yes", NULL, 0, 0},
    UX_MENU_END};

const ux_menu_entry_t menu_settings[] = {
    {NULL, menu_settings_browser_init, 0, NULL, "Browser support", NULL, 0, 0},
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
    UX_MENU_END};
#endif // HAVE_U2F

const ux_menu_entry_t menu_about[] = {
    {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
#ifdef HAVE_U2F
    {menu_main, NULL, 2, &C_nanos_icon_back, "Back", NULL, 61, 40},
#else
    {menu_main, NULL, 1, &C_nanos_icon_back, "Back", NULL, 61, 40},
#endif // HAVE_U2F
    UX_MENU_END};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_nanos_badge_raiblocks, "Use wallet to",
     "view accounts", 33, 12},
#ifdef HAVE_U2F
    {menu_settings, NULL, 0, NULL, "Settings", NULL, 0, 0},
#endif // HAVE_U2F
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
     /* text */ vars.display_address.account, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

const bagl_element_t *ui_display_address_prepro(const bagl_element_t *element) {
    if (element->component.userid > 0) {
        unsigned int display = (ux_step == element->component.userid - 1);
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
    os_memset(&vars.display_address, 0, sizeof(vars.display_address));
    // Encode public key into an address string
    rai_write_account_string((uint8_t *)vars.display_address.account, rai_public_key_D);
    vars.display_address.account[ACCOUNT_STRING_LEN] = '\0';

    ux_step_count = 2;
    ux_step = 0;
    UX_DISPLAY(ui_display_address, ui_display_address_prepro);
}

#endif // defined(TARGET_NANOS)
