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

#ifndef IS_STANDALONE_APP

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

#elif defined(DEFAULT_COIN_TYPE_LIBN_COIN_TYPE_NANO)

#define COIN_NAME "Nano"
#define COIN_BADGE &C_nanos_badge_nano
#define COIN_BIP32_PREFIX ((uint32_t [2]){ HARDENED(44), HARDENED(165) })
#define COIN_PRIMARY_PREFIX "nano_"
#define COIN_SECONDARY_PREFIX "xrb_"
#define COIN_DEFAULT_PREFIX LIBN_SECONDARY_PREFIX
#define COIN_UNIT "NANO"
#define COIN_UNIT_SCALE 30

#elif defined(DEFAULT_COIN_TYPE_LIBN_COIN_TYPE_BANANO)

#define COIN_NAME "Banano"
#define COIN_BADGE &C_nanos_badge_banano
#define COIN_BIP32_PREFIX ((uint32_t [2]){ HARDENED(44), HARDENED(198) })
#define COIN_PRIMARY_PREFIX "ban_"
#define COIN_SECONDARY_PREFIX "ban_"
#define COIN_DEFAULT_PREFIX LIBN_PRIMARY_PREFIX
#define COIN_UNIT "BANANO"
#define COIN_UNIT_SCALE 29

#endif

#endif // COINS_H