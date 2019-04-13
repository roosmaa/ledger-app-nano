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

#ifndef COINS_H
#define COINS_H

#include "libn_types.h"

#define HARDENED(x) ((x) + 0x80000000)

extern libn_coin_conf_t libn_coin_conf_D;

void init_coin_config(libn_coin_type_t coin_type);

#define COIN_NAME libn_coin_conf_D.coinName
#define COIN_BADGE libn_coin_conf_D.coinBadge
#define COIN_BIP32_PREFIX libn_coin_conf_D.bip32Prefix
#define COIN_PRIMARY_PREFIX libn_coin_conf_D.addressPrimaryPrefix
#define COIN_SECONDARY_PREFIX libn_coin_conf_D.addressSecondaryPrefix
#define COIN_DEFAULT_PREFIX libn_coin_conf_D.addressDefaultPrefix
#define COIN_UNIT libn_coin_conf_D.defaultUnit
#define COIN_UNIT_SCALE libn_coin_conf_D.defaultUnitScale
#if defined(TARGET_BLUE)
#define COIN_COLOR_BG libn_coin_conf_D.colorBackground
#define COIN_COLOR_FG libn_coin_conf_D.colorForeground
#define COIN_COLOR_ALT_BG libn_coin_conf_D.colorAltBackground
#define COIN_COLOR_ALT_FG libn_coin_conf_D.colorAltForeground
#define COIN_COLOR_REJECT_BG libn_coin_conf_D.colorRejectBackground
#define COIN_COLOR_REJECT_FG libn_coin_conf_D.colorRejectForeground
#define COIN_COLOR_REJECT_OVER_BG libn_coin_conf_D.colorRejectOverBackground
#define COIN_COLOR_REJECT_OVER_FG libn_coin_conf_D.colorRejectOverForeground
#define COIN_COLOR_CONFIRM_BG libn_coin_conf_D.colorConfirmBackground
#define COIN_COLOR_CONFIRM_FG libn_coin_conf_D.colorConfirmForeground
#define COIN_COLOR_CONFIRM_OVER_BG libn_coin_conf_D.colorConfirmOverBackground
#define COIN_COLOR_CONFIRM_OVER_FG libn_coin_conf_D.colorConfirmOverForeground
#define COIN_ICON_TOGGLE_OFF libn_coin_conf_D.iconToggleOff
#define COIN_ICON_TOGGLE_ON libn_coin_conf_D.iconToggleOn
#endif // TARGET_BLUE

#endif // COINS_H