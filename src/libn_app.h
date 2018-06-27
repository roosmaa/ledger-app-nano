/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
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

#ifndef LIBN_H

#define LIBN_H

#include "os.h"

#define L_DEBUG_APP(x)
#define L_DEBUG_NOPREFIX(x)
#define L_DEBUG_BUF(x)

#define SW_TECHNICAL_DETAILS(x) LIBN_SW_TECHNICAL_PROBLEM

#include "libn_secure_value.h"
#include "libn_types.h"

void app_async_response(libn_apdu_response_t *resp, uint16_t statusWord);
bool app_apply_state(void);

void app_init(void);
void app_main(void);
void app_exit(void);

#endif
