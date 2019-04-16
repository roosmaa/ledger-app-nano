/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2016 Ledger
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

#ifndef LIBN_PUBLIC_RAM_VARIABLES_H

#define LIBN_PUBLIC_RAM_VARIABLES_H

#include "blake2b.h"

#include "libn_context.h"

/** Buffer used for asynchronous response data **/
extern uint8_t libn_async_buffer_D[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];
extern libn_context_t libn_context_D;

#endif // LIBN_PUBLIC_RAM_VARIABLES_H
