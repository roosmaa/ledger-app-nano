/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
*   (c) 2016 Ledger
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

#include "rai_internal.h"

#define DISPATCHER_APDUS 3

typedef uint16_t (*apduProcessingFunction)(void);

extern uint8_t const DISPATCHER_CLA[DISPATCHER_APDUS];
extern uint8_t const DISPATCHER_INS[DISPATCHER_APDUS];
extern bool const DISPATCHER_DATA_IN[DISPATCHER_APDUS];
extern apduProcessingFunction const DISPATCHER_FUNCTIONS[DISPATCHER_APDUS];

#define RAI_ACCOUNT_STRING_BASE_LEN 60

extern uint8_t const BASE16_ALPHABET[16];

extern uint8_t const BASE32_ALPHABET[32];
extern uint8_t const BASE32_TABLE[75];
