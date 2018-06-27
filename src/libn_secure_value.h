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

#ifndef LIBN_SECURE_VALUE_H

#define LIBN_SECURE_VALUE_H

#include "os.h"

typedef uint16_t secu8;
typedef uint32_t secu16;

void sbSet(secu8 *target, uint8_t source);
void sbCheck(secu8 source);
void ssSet(secu16 *target, uint16_t source);
void ssCheck(secu16 source);

#define SB_GET(x) ((uint8_t)x)

#define SB_SET(x, y) sbSet(&x, y);

#define SB_CHECK(x) sbCheck(x);

#define SS_GET(x) ((uint16_t)x)

#define SS_SET(x, y) ssSet(&x, y);

#define SS_CHECK(x) ssCheck(x);

#endif
