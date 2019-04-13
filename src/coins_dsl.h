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

#ifndef COINS_DSL_H
#define COINS_DSL_H

#include "coins.h"
#include "libn_internal.h"
#include "glyphs.h"

#define REGISTER_COINS(...)                                                   \
    void init_coin_config(libn_coin_type_t coin_type) {                       \
        switch (coin_type) {                                                  \
        __VA_ARGS__                                                           \
        }                                                                     \
        /* avoid default statement to get warnings when a case was missed */  \
        app_exit();                                                           \
    }

#define COIN(type, ...)                                                       \
    case type: {                                                              \
        libn_coin_conf_t conf = __VA_ARGS__;                                  \
        os_memmove(&libn_coin_conf_D, &conf, sizeof(libn_coin_conf_t));       \
        return;                                                               \
    }

#endif // COINS_DSL_H