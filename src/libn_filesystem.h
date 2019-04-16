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

#ifndef LIBN_FS_H

#define LIBN_FS_H

#include <stdbool.h>

#include "os.h"
#include "libn_context.h"

typedef struct libn_storage_s {
    bool autoReceive;
} libn_storage_t;

// the global nvram memory variable
extern libn_storage_t const N_libn_real;
#define N_libn (*(volatile libn_storage_t *)PIC(&N_libn_real))

void libn_set_auto_receive(bool enabled);

#endif
