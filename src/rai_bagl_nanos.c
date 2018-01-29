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

const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_settings[];
const ux_menu_entry_t menu_settings_browser[];

#ifdef HAVE_U2F
// change the setting
void menu_settings_browser_change(unsigned int enabled) {
    rai_set_fido_transport(enabled);
    USB_power_U2F(false, false);
    USB_power_U2F(true, N_rai.fidoTransport);
    // go back to the menu entry
    UX_MENU_DISPLAY(0, menu_settings, NULL);
}

// show the currently activated entry
void menu_settings_browser_init(unsigned int ignored) {
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

void rai_bagl_display_address(void) {
    // TODO: Display the actual UI
}

#endif // defined(TARGET_NANOS)
