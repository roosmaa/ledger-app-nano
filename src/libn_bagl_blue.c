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

#if defined(TARGET_BLUE)

typedef enum {
    LIBN_IDLE_STATE_HOME,
    LIBN_IDLE_STATE_SETTINGS,
} libn_idle_state_t;

extern ux_state_t ux;

libn_state_t bagl_state;
libn_idle_state_t bagl_idle_state;

#define ACCOUNT_BUF_LEN ( \
    LIBN_ACCOUNT_STRING_BASE_LEN \
    + MAX(sizeof(COIN_PRIMARY_PREFIX), \
          sizeof(COIN_SECONDARY_PREFIX)) \
    + 1 \
)

#define OPEN_LABEL_PREFIX "Open "
#define OPEN_LABEL_SUFFIX " wallet"

char header_text[sizeof(COIN_NAME)+1];
union {
    struct {
        char openLabel[sizeof(OPEN_LABEL_PREFIX)
            + sizeof(COIN_NAME)
            + sizeof(OPEN_LABEL_SUFFIX)
            + 1];
    } idle;
    struct {
        bagl_element_t autoReceiveToggle;
    } settings;
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

const bagl_element_t *ui_touch_settings(const bagl_element_t *e);
const bagl_element_t *ui_touch_exit(const bagl_element_t *e);
const bagl_element_t *ui_touch_back(const bagl_element_t *e);
const bagl_element_t *ui_touch_auto_receive(const bagl_element_t *e);

const bagl_element_t ui_idle[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_ALT_BG, /* bgcolor */ COIN_COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_BG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COIN_COLOR_ALT_FG, /* bgcolor */ COIN_COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ header_text, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 19, /* width */ 56, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_ALT_BG, /* bgcolor */ COIN_COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_SETTINGS, /* touch_area_brim */ 0,
     /* overfgcolor */ COIN_COLOR_ALT_BG, /* overbgcolor */ COIN_COLOR_ALT_FG,
     /* tap */ ui_touch_settings, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 264, /* y */ 19, /* width */ 56, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_ALT_BG, /* bgcolor */ COIN_COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_DASHBOARD, /* touch_area_brim */ 0,
     /* overfgcolor */ COIN_COLOR_ALT_BG, /* overbgcolor */ COIN_COLOR_ALT_FG,
     /* tap */ ui_touch_exit, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_ICON, /* userid */ 0x00,
      /* x */ 135, /* y */ 178, /* width */ 50, /* height */ 50,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ (char *)COIN_BADGE, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 270, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_LIGHT_16_22PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.idle.openLabel, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 308, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Connect the Ledger Blue and open your", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 331, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "preferred wallet to view your accounts.", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 450, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Approval requests will show automatically.", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

uint32_t ui_idle_button(uint32_t button_mask,
                        uint32_t button_mask_counter) {
    return 0;
}

void libn_bagl_idle(void) {
    bagl_state = LIBN_STATE_READY;
    bagl_idle_state = LIBN_IDLE_STATE_HOME;
    UX_SET_STATUS_BAR_COLOR(COIN_COLOR_ALT_FG, COIN_COLOR_ALT_BG);

    // Uppercase the coin name
    os_memset(header_text, 0, sizeof(header_text));
    strncpy(header_text, COIN_NAME, MIN(sizeof(header_text)-1, sizeof(COIN_NAME)));
    for (size_t i = 0; i < sizeof(header_text); i++) {
        if (header_text[i] >= 'a' && header_text[i] <= 'z') {
            header_text[i] = 'A' + header_text[i] - 'a';
        }
    }

    // Concat OPEN_LABEL_PREFIX + COIN_NAME + OPEN_LABEL_SUFFIX
    os_memset(vars.idle.openLabel, 0, sizeof(vars.idle.openLabel));
    size_t max = sizeof(vars.idle.openLabel) - 1;
    char *ptr = vars.idle.openLabel;

    strncat(ptr, OPEN_LABEL_PREFIX, MIN(max, sizeof(OPEN_LABEL_PREFIX)));
    max = MAX(0, max - strlen(OPEN_LABEL_PREFIX));
    ptr += strlen(OPEN_LABEL_PREFIX);
    strncat(ptr, COIN_NAME, MIN(max, sizeof(COIN_NAME)));
    max = MAX(0, max - strlen(COIN_NAME));
    ptr += strlen(COIN_NAME);
    strncat(ptr, OPEN_LABEL_SUFFIX, MIN(max, sizeof(OPEN_LABEL_SUFFIX)));
    max = MAX(0, max - strlen(OPEN_LABEL_SUFFIX));
    ptr += strlen(OPEN_LABEL_SUFFIX);

    UX_DISPLAY(ui_idle, NULL);
}

const bagl_element_t ui_settings[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_ALT_BG, /* bgcolor */ COIN_COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_BG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COIN_COLOR_ALT_FG, /* bgcolor */ COIN_COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "SETTINGS", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 19, /* width */ 50, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_ALT_BG, /* bgcolor */ COIN_COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_LEFT, /* touch_area_brim */ 0,
     /* overfgcolor */ COIN_COLOR_ALT_BG, /* overbgcolor */ COIN_COLOR_ALT_FG,
     /* tap */ ui_touch_back, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 105, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Auto-receive", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 126, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "No confirmation for receive transactions", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_NONE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 78, /* width */ 320, /* height */ 68,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ 0, /* scrollspeed */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ COIN_COLOR_ALT_BG, /* overbgcolor */ COIN_COLOR_ALT_FG,
     /* tap */ ui_touch_auto_receive, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 329, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Version", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 350, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ APPVERSION, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 379, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Developer", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 400, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "Mart Roosmaa", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 429, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Source code", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 450, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "https://github.com/roosmaa/blue-app-nano", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Quick-redrawable icons
    {{/* type */ BAGL_ICON, /* userid */ 0x01,
      /* x */ 258, /* y */ 98, /* width */ 32, /* height */ 18,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COIN_COLOR_FG, /* bgcolor */ COIN_COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ (char *)&C_blue_icon_toggle_off, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

uint32_t ui_settings_button(uint32_t button_mask,
                            uint32_t button_mask_counter) {
    return 0;
}

const bagl_element_t *ui_settings_prepro(const bagl_element_t *e) {
    if ((e->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return NULL;
    }

    switch (e->component.userid) {
    case 0x01:
        os_memmove(&vars.settings.autoReceiveToggle, e, sizeof(bagl_element_t));
        vars.settings.autoReceiveToggle.text = (const char *)(N_libn.autoReceive
            ? &C_blue_icon_toggle_on
            : &C_blue_icon_toggle_off);
        return &vars.settings.autoReceiveToggle;
    }

    return e;
}

void libn_bagl_settings(void) {
    bagl_state = LIBN_STATE_READY;
    bagl_idle_state = LIBN_IDLE_STATE_SETTINGS;

    const bool quickRedaw = UX_DISPLAYED()
        && ux.elements == ui_settings
        && ux.elements_preprocessor == ui_settings_prepro;

    if (!quickRedaw) {
        UX_DISPLAY(ui_settings, ui_settings_prepro);
    } else {
        // Only redisplay the toggle buttons for a faster screen refresh
        const size_t totalElements = sizeof(ui_settings) / sizeof(bagl_element_t);
        UX_REDISPLAY_IDX(totalElements - 1);
    }
}

void ui_ticker_event(bool uxAllowed) {
    // TODO
}

bool libn_bagl_apply_state() {
    if (!UX_DISPLAYED()) {
        return false;
    }

    switch (libn_context_D.state) {
    case LIBN_STATE_READY:
        if (bagl_state != LIBN_STATE_READY) {
            switch (bagl_idle_state) {
            case LIBN_IDLE_STATE_HOME:
                libn_bagl_idle();
                return true;
            case LIBN_IDLE_STATE_SETTINGS:
                libn_bagl_settings();
                return true;
            }
        }
        break;
    case LIBN_STATE_CONFIRM_ADDRESS:
        if (bagl_state != LIBN_STATE_CONFIRM_ADDRESS) {
            // TODO
            libn_bagl_idle();
            // libn_bagl_display_address();
            return true;
        }
        break;
    case LIBN_STATE_CONFIRM_SIGNATURE:
        if (bagl_state != LIBN_STATE_CONFIRM_SIGNATURE) {
            // TODO
            libn_bagl_idle();
            // libn_bagl_confirm_sign_block();
            return true;
        }
        break;
    }

    return false;
}

const bagl_element_t *ui_touch_settings(const bagl_element_t *e) {
    libn_bagl_settings();
    return NULL;
}

const bagl_element_t *ui_touch_exit(const bagl_element_t *e) {
    os_sched_exit(0);
    return NULL;
}

const bagl_element_t *ui_touch_back(const bagl_element_t *e) {
    if (bagl_state == LIBN_STATE_READY && bagl_idle_state == LIBN_IDLE_STATE_SETTINGS) {
        libn_bagl_idle();
    }
    return NULL;
}

const bagl_element_t *ui_touch_auto_receive(const bagl_element_t *e) {
    libn_set_auto_receive(!N_libn.autoReceive);
    libn_bagl_settings();
    return NULL;
}

#endif
