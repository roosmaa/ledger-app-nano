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

#include "libn_internal.h"

void sbSet(secu8 *target, uint8_t source) {
    *target = (((uint16_t)~source) << 8) + source;
}

void sbCheck(secu8 source) {
    if (((source >> 8) & 0xff) != (uint8_t)(~(source & 0xff))) {
        reset();
    }
}

void ssSet(secu16 *target, uint16_t source) {
    *target = (((uint32_t)~source) << 16) + source;
}

void ssCheck(secu16 source) {
    if (((source >> 16) & 0xffff) != (uint16_t)(~(source & 0xffff))) {
        reset();
    }
}
