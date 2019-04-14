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

#define COIN_MAX_PREFIX ( \
    MAX(sizeof(COIN_PRIMARY_PREFIX), \
        sizeof(COIN_SECONDARY_PREFIX)) \
)
#define ACCOUNT_BUF_LEN ( \
    LIBN_ACCOUNT_STRING_BASE_LEN \
    + COIN_MAX_PREFIX \
    + 1 \
)

#define OPEN_LABEL_PREFIX "Open "
#define OPEN_LABEL_SUFFIX " wallet"

typedef union {
    uint8_t buf[ACCOUNT_BUF_LEN + 1];
    struct {
        char first[ACCOUNT_BUF_LEN - LIBN_ACCOUNT_STRING_BASE_LEN / 2];
        char second[LIBN_ACCOUNT_STRING_BASE_LEN / 2 + 1];
    } lines;
} ui_split_address_t;

void ui_write_split_address(const libn_address_formatter_t *fmt,
                            ui_split_address_t *address,
                            const libn_public_key_t publicKey) {
    const size_t addressLen = libn_address_format(fmt, address->buf, publicKey);
    address->buf[addressLen] = '\0';
    // Move the last 30 characters to the 2nd line memory space
    address->lines.second[sizeof(address->lines.second) - 1] = '\0';
    for (size_t i = 1; i < sizeof(address->lines.second); i++) {
        const size_t di = sizeof(address->lines.second) - 1 - i;
        const size_t si = addressLen - 1 - (i - 1);
        address->lines.second[di] = address->buf[si];
        address->buf[si] = '\0';
    }
}

typedef char ui_truncated_address_t[13 + COIN_MAX_PREFIX];

void ui_write_address_truncated(const libn_address_formatter_t *fmt,
                                ui_truncated_address_t label,
                                const libn_public_key_t publicKey) {
    uint8_t buf[ACCOUNT_BUF_LEN];
    const size_t addressLen = libn_address_format(fmt, buf, publicKey);
    const size_t prefixLen = addressLen - LIBN_ACCOUNT_STRING_BASE_LEN;

    os_memmove(label, buf, prefixLen + 5);
    os_memset(label + prefixLen + 5, '.', 2);
    os_memmove(label + prefixLen + 7, buf + addressLen - 5, 5);
    label[prefixLen+12] = '\0';
}

typedef char ui_truncated_hash_t[13];

void ui_write_hash_truncated(ui_truncated_hash_t label, libn_hash_t hash) {
    uint8_t buf[sizeof(libn_hash_t) * 2];
    libn_write_hex_string(buf, hash, sizeof(libn_hash_t));
    // Truncate hash to 12345..67890 format
    os_memmove(label, buf, 5);
    os_memset(label+5, '.', 2);
    os_memmove(label+7, buf+sizeof(buf)-5, 5);
    label[12] = '\0';
}

#define SEND_AMOUNT_LABEL "Send amount"
#define RECEIVE_AMOUNT_LABEL "Receive amount"

bagl_element_t mutableElement;

union {
    struct {
        char appTitle[sizeof(COIN_NAME)+1];
        char openLabel[sizeof(OPEN_LABEL_PREFIX)
            + sizeof(COIN_NAME)
            + sizeof(OPEN_LABEL_SUFFIX)
            + 1];
    } idle;
    struct {
    } settings;
    struct {
        ui_split_address_t address;
    } displayAddress;
    struct {
        ui_truncated_address_t accountAddress;
        char amountLabel[MAX(sizeof(SEND_AMOUNT_LABEL), sizeof(RECEIVE_AMOUNT_LABEL))];
        char amountValue[40];
        ui_split_address_t recipientAddress;
        ui_split_address_t representativeAddress;
        ui_truncated_hash_t blockHash;
    } confirmSignBlock;
} vars;

#define COLOR_BG               0x001001
#define COLOR_FG               0x001002
#define COLOR_ALT_BG           0x001003
#define COLOR_ALT_FG           0x001004
#define COLOR_REJECT_BG        0x001005
#define COLOR_REJECT_FG        0x001006
#define COLOR_REJECT_OVER_BG   0x001007
#define COLOR_REJECT_OVER_FG   0x001008
#define COLOR_CONFIRM_BG       0x001009
#define COLOR_CONFIRM_FG       0x001010
#define COLOR_CONFIRM_OVER_BG  0x001011
#define COLOR_CONFIRM_OVER_FG  0x001012

#define REMAP_TO_COIN_COLORS(variable) switch (variable) {                    \
    case COLOR_BG: variable = COIN_COLOR_BG; break;                           \
    case COLOR_FG: variable = COIN_COLOR_FG; break;                           \
    case COLOR_ALT_BG: variable = COIN_COLOR_ALT_BG; break;                   \
    case COLOR_ALT_FG: variable = COIN_COLOR_ALT_FG; break;                   \
    case COLOR_REJECT_BG: variable = COIN_COLOR_REJECT_BG; break;             \
    case COLOR_REJECT_FG: variable = COIN_COLOR_REJECT_FG; break;             \
    case COLOR_REJECT_OVER_BG: variable = COIN_COLOR_REJECT_OVER_BG; break;   \
    case COLOR_REJECT_OVER_FG: variable = COIN_COLOR_REJECT_OVER_FG; break;   \
    case COLOR_CONFIRM_BG: variable = COIN_COLOR_CONFIRM_BG; break;           \
    case COLOR_CONFIRM_FG: variable = COIN_COLOR_CONFIRM_FG; break;           \
    case COLOR_CONFIRM_OVER_BG: variable = COIN_COLOR_CONFIRM_OVER_BG; break; \
    case COLOR_CONFIRM_OVER_FG: variable = COIN_COLOR_CONFIRM_OVER_FG; break; \
    default: break;                                                           \
}

const bagl_element_t *ui_touch_settings(const bagl_element_t *e);
const bagl_element_t *ui_touch_exit(const bagl_element_t *e);
const bagl_element_t *ui_touch_back(const bagl_element_t *e);
const bagl_element_t *ui_touch_auto_receive(const bagl_element_t *e);
const bagl_element_t *ui_touch_reject(const bagl_element_t *e);
const bagl_element_t *ui_touch_confirm(const bagl_element_t *e);

const bagl_element_t ui_idle[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x01,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_BG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COLOR_ALT_FG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.idle.appTitle, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 19, /* width */ 56, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_SETTINGS, /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_ALT_BG, /* overbgcolor */ COLOR_ALT_FG,
     /* tap */ ui_touch_settings, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 264, /* y */ 19, /* width */ 56, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_DASHBOARD, /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_ALT_BG, /* overbgcolor */ COLOR_ALT_FG,
     /* tap */ ui_touch_exit, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_ICON, /* userid */ 0x01,
      /* x */ 135, /* y */ 178, /* width */ 50, /* height */ 50,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 270, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_LIGHT_16_22PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.idle.openLabel, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 308, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Connect the Ledger Blue and open your", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 331, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "preferred wallet to view your accounts.", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 450, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Approval requests will show automatically.", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
};

const bagl_element_t *ui_idle_prepro(const bagl_element_t *e) {
    if ((e->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return NULL;
    }

    os_memmove(&mutableElement, e, sizeof(bagl_element_t));
    REMAP_TO_COIN_COLORS(mutableElement.component.fgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.component.bgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overfgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overbgcolor);

    switch (mutableElement.component.userid) {
    case 0x01:
        mutableElement.text = (char *)COIN_BADGE;
        break;
    }

    return &mutableElement;
}

uint32_t ui_idle_button(uint32_t button_mask,
                        uint32_t button_mask_counter) {
    return 0;
}

void libn_bagl_idle(void) {
    bagl_state = LIBN_STATE_READY;
    bagl_idle_state = LIBN_IDLE_STATE_HOME;
    UX_SET_STATUS_BAR_COLOR(COIN_COLOR_ALT_FG, COIN_COLOR_ALT_BG);

    // Uppercase the coin name
    os_memset(vars.idle.appTitle, 0, sizeof(vars.idle.appTitle));
    strncpy(vars.idle.appTitle, COIN_NAME, MIN(sizeof(vars.idle.appTitle)-1, sizeof(COIN_NAME)));
    for (size_t i = 0; i < sizeof(vars.idle.appTitle); i++) {
        if (vars.idle.appTitle[i] >= 'a' && vars.idle.appTitle[i] <= 'z') {
            vars.idle.appTitle[i] = 'A' + vars.idle.appTitle[i] - 'a';
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

    UX_DISPLAY(ui_idle, ui_idle_prepro);
}

const bagl_element_t ui_settings[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_BG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COLOR_ALT_FG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "SETTINGS", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 19, /* width */ 50, /* height */ 44,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_FG,
      /* font_id */ BAGL_FONT_SYMBOLS_0 | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* scrollspeed */ 0},
     /* text */ BAGL_FONT_SYMBOLS_0_LEFT, /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_ALT_BG, /* overbgcolor */ COLOR_ALT_FG,
     /* tap */ ui_touch_back, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 105, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Auto-receive", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 126, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "No confirmation for receive transactions", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_NONE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 78, /* width */ 320, /* height */ 68,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* scrollspeed */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_ALT_BG, /* overbgcolor */ COLOR_ALT_FG,
     /* tap */ ui_touch_auto_receive, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 329, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Version", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 350, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ APPVERSION, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 379, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Developer", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 400, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "Mart Roosmaa", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 429, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX,
      /* scrollspeed */ 0},
     /* text */ "Source code", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 450, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_8_11PX,
      /* scrollspeed */ 0},
     /* text */ "https://github.com/roosmaa/blue-app-nano", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Quick-redrawable icons
    {{/* type */ BAGL_ICON, /* userid */ 0x01,
      /* x */ 258, /* y */ 98, /* width */ 32, /* height */ 18,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
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

    os_memmove(&mutableElement, e, sizeof(bagl_element_t));
    REMAP_TO_COIN_COLORS(mutableElement.component.fgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.component.bgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overfgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overbgcolor);

    switch (e->component.userid) {
    case 0x01:
        mutableElement.text = (const char *)(N_libn.autoReceive
            ? COIN_ICON_TOGGLE_ON
            : COIN_ICON_TOGGLE_OFF);
        break;
    }

    return &mutableElement;
}

void libn_bagl_settings(void) {
    if (libn_context_D.state != LIBN_STATE_READY) {
        return;
    }

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

const bagl_element_t ui_confirm_address[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_BG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COLOR_ALT_FG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "CONFIRM ADDRESS", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 105, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Account address", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 131, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.displayAddress.address.lines.first, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 30, /* y */ 157, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.displayAddress.address.lines.second, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 40, /* y */ 414, /* width */ 115, /* height */ 36,
      /* stroke */ 0, /* radius */ 18, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_REJECT_BG, /* bgcolor */ COLOR_REJECT_FG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* icon_id */ 0},
     /* text */ "REJECT", /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_REJECT_OVER_BG, /* overbgcolor */ COLOR_REJECT_OVER_FG,
     /* tap */ ui_touch_reject, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 165, /* y */ 414, /* width */ 115, /* height */ 36,
      /* stroke */ 0, /* radius */ 18, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_CONFIRM_BG, /* bgcolor */ COLOR_CONFIRM_FG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* icon_id */ 0},
     /* text */ "CONFIRM", /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_CONFIRM_OVER_BG, /* overbgcolor */ COLOR_CONFIRM_OVER_FG,
     /* tap */ ui_touch_confirm, /* out */ NULL, /* over */ NULL},
};

const bagl_element_t *ui_confirm_address_prepro(const bagl_element_t *e) {
    if ((e->component.type & (~BAGL_FLAG_TOUCHABLE)) == BAGL_NONE) {
        return NULL;
    }

    os_memmove(&mutableElement, e, sizeof(bagl_element_t));
    REMAP_TO_COIN_COLORS(mutableElement.component.fgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.component.bgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overfgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overbgcolor);

    return &mutableElement;
}

uint32_t ui_confirm_address_button(uint32_t button_mask,
                                   uint32_t button_mask_counter) {
    return 0;
}

void libn_bagl_confirm_address(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_ADDRESS) {
        return;
    }
    libn_apdu_get_address_request_t *req = &libn_context_D.stateData.getAddressRequest;

    os_memset(&vars.displayAddress, 0, sizeof(vars.displayAddress));
    // Encode public key into an address string
    ui_write_split_address(
        &req->addressFormatter,
        &vars.displayAddress.address,
        req->publicKey);

    bagl_state = LIBN_STATE_CONFIRM_ADDRESS;
    UX_DISPLAY(ui_confirm_address, ui_confirm_address_prepro);
}

const bagl_element_t ui_confirm_block[] = {
    // Header background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 20, /* width */ 320, /* height */ 48,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_ALT_BG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content background
    {{/* type */ BAGL_RECTANGLE, /* userid */ 0x00,
      /* x */ 0, /* y */ 68, /* width */ 320, /* height */ 413,
      /* stroke */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_BG, /* bgcolor */ COLOR_BG,
      /* font_id */ 0, /* icon_id */ 0},
     /* text */ NULL, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Header views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x00,
      /* x */ 0, /* y */ 45, /* width */ 320, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ 0,
      /* fgcolor */ COLOR_ALT_FG, /* bgcolor */ COLOR_ALT_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "CONFIRM BLOCK", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    // Content views
    {{/* type */ BAGL_LABELINE, /* userid */ 0x10,
      /* x */ 30, /* y */ 105, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Your account", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x11,
      /* x */ 30, /* y */ 131, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.accountAddress, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x20,
      /* x */ 30, /* y */ 160, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.amountLabel, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x21,
      /* x */ 30, /* y */ 186, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.amountValue, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x30,
      /* x */ 30, /* y */ 215, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Send to", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x31,
      /* x */ 30, /* y */ 241, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.recipientAddress.lines.first,
     /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x32,
      /* x */ 30, /* y */ 262, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.recipientAddress.lines.second,
     /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x40,
      /* x */ 30, /* y */ 291, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Representative", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x41,
      /* x */ 30, /* y */ 317, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.representativeAddress.lines.first,
     /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x42,
      /* x */ 30, /* y */ 338, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.representativeAddress.lines.second,
     /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_LABELINE, /* userid */ 0x50,
      /* x */ 30, /* y */ 367, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_SEMIBOLD_8_11PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ "Block hash", /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_LABELINE, /* userid */ 0x51,
      /* x */ 30, /* y */ 393, /* width */ 260, /* height */ 30,
      /* scrolldelay */ 0, /* radius */ 0, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_FG, /* bgcolor */ COLOR_BG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_10_13PX | BAGL_FONT_ALIGNMENT_CENTER,
      /* scrollspeed */ 0},
     /* text */ vars.confirmSignBlock.blockHash, /* touch_area_brim */ 0,
     /* overfgcolor */ 0, /* overbgcolor */ 0,
     /* tap */ NULL, /* out */ NULL, /* over */ NULL},

    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 40, /* y */ 414, /* width */ 115, /* height */ 36,
      /* stroke */ 0, /* radius */ 18, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_REJECT_BG, /* bgcolor */ COLOR_REJECT_FG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* icon_id */ 0},
     /* text */ "REJECT", /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_REJECT_OVER_BG, /* overbgcolor */ COLOR_REJECT_OVER_FG,
     /* tap */ ui_touch_reject, /* out */ NULL, /* over */ NULL},
    {{/* type */ BAGL_RECTANGLE | BAGL_FLAG_TOUCHABLE, /* userid */ 0x00,
      /* x */ 165, /* y */ 414, /* width */ 115, /* height */ 36,
      /* stroke */ 0, /* radius */ 18, /* fill */ BAGL_FILL,
      /* fgcolor */ COLOR_CONFIRM_BG, /* bgcolor */ COLOR_CONFIRM_FG,
      /* font_id */ BAGL_FONT_OPEN_SANS_REGULAR_11_14PX | BAGL_FONT_ALIGNMENT_CENTER | BAGL_FONT_ALIGNMENT_MIDDLE,
      /* icon_id */ 0},
     /* text */ "CONFIRM", /* touch_area_brim */ 0,
     /* overfgcolor */ COLOR_CONFIRM_OVER_BG, /* overbgcolor */ COLOR_CONFIRM_OVER_FG,
     /* tap */ ui_touch_confirm, /* out */ NULL, /* over */ NULL},
};

uint32_t ui_confirm_block_button(uint32_t button_mask,
                                   uint32_t button_mask_counter) {
    return 0;
}

const bagl_element_t *ui_confirm_block_prepro(const bagl_element_t *e) {
    const bool showAmount = vars.confirmSignBlock.amountValue[0] != '\0';
    const bool showRecipient = vars.confirmSignBlock.recipientAddress.buf[0] != '\0';
    const bool showRepresentative = vars.confirmSignBlock.representativeAddress.buf[0] != '\0';

    os_memmove(&mutableElement, e, sizeof(bagl_element_t));
    REMAP_TO_COIN_COLORS(mutableElement.component.fgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.component.bgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overfgcolor);
    REMAP_TO_COIN_COLORS(mutableElement.overbgcolor);

    uint16_t y = 97;

    #define LAYOUT(el_uid, el_vis) {                         \
        if (el_vis) {                                        \
            const bool isHeader = (el_uid & 0x0F) == 0;      \
            y += isHeader ? 8 : 0;                           \
            if (mutableElement.component.userid == el_uid) { \
                mutableElement.component.y = y;              \
                return &mutableElement;                      \
            }                                                \
            y += (isHeader ? 5 : 0) + 21;                    \
        } else if (e->component.userid == el_uid) {          \
            return NULL;                                     \
        }                                                    \
    }

    // Account
    LAYOUT(0x10, true);
    LAYOUT(0x11, true);
    // Amount
    LAYOUT(0x20, showAmount);
    LAYOUT(0x21, showAmount);
    // Recipient
    LAYOUT(0x30, showRecipient);
    LAYOUT(0x31, showRecipient);
    LAYOUT(0x32, showRecipient);
    // Representative
    LAYOUT(0x40, showRepresentative);
    LAYOUT(0x41, showRepresentative);
    LAYOUT(0x42, showRepresentative);
    // Block hash
    LAYOUT(0x50, true);
    LAYOUT(0x51, true);

    #undef LAYOUT

    return &mutableElement;
}

void libn_bagl_confirm_block(void) {
    if (libn_context_D.state != LIBN_STATE_CONFIRM_SIGNATURE) {
        return;
    }
    libn_apdu_sign_block_request_t *req = &libn_context_D.stateData.signBlockRequest;

    os_memset(&vars.confirmSignBlock, 0, sizeof(vars.confirmSignBlock));

    ui_write_address_truncated(
        &req->addressFormatter,
        vars.confirmSignBlock.accountAddress,
        req->publicKey);

    if (!libn_is_zero(req->amount, sizeof(req->amount))) {
        libn_amount_format(
            &req->amountFormatter,
            vars.confirmSignBlock.amountValue,
            sizeof(vars.confirmSignBlock.amountValue),
            req->amount);

        if (!libn_is_zero(req->recipient, sizeof(req->recipient))) {
            strcpy(vars.confirmSignBlock.amountLabel, SEND_AMOUNT_LABEL);
            ui_write_split_address(
                &req->recipientFormatter,
                &vars.confirmSignBlock.recipientAddress,
                req->recipient);
        } else {
            strcpy(vars.confirmSignBlock.amountLabel, RECEIVE_AMOUNT_LABEL);
        }
    }

    if (!libn_is_zero(req->representative, sizeof(req->representative))) {
        ui_write_split_address(
            &req->representativeFormatter,
            &vars.confirmSignBlock.representativeAddress,
            req->representative);
    }

    ui_write_hash_truncated(
        vars.confirmSignBlock.blockHash,
        req->blockHash);

    bagl_state = LIBN_STATE_CONFIRM_SIGNATURE;
    UX_DISPLAY(ui_confirm_block, ui_confirm_block_prepro);
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
            libn_bagl_confirm_address();
            return true;
        }
        break;
    case LIBN_STATE_CONFIRM_SIGNATURE:
        if (bagl_state != LIBN_STATE_CONFIRM_SIGNATURE) {
            libn_bagl_confirm_block();
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

const bagl_element_t *ui_touch_reject(const bagl_element_t *e) {
    switch (bagl_state) {
    case LIBN_STATE_READY: break;
    case LIBN_STATE_CONFIRM_ADDRESS:
        libn_bagl_display_address_callback(false);
        break;
    case LIBN_STATE_CONFIRM_SIGNATURE:
        libn_bagl_confirm_sign_block_callback(false);
        break;
    }
    return NULL;
}

const bagl_element_t *ui_touch_confirm(const bagl_element_t *e) {
    switch (bagl_state) {
    case LIBN_STATE_READY: break;
    case LIBN_STATE_CONFIRM_ADDRESS:
        libn_bagl_display_address_callback(true);
        break;
    case LIBN_STATE_CONFIRM_SIGNATURE:
        libn_bagl_confirm_sign_block_callback(true);
        break;
    }
    return NULL;
}

#endif
