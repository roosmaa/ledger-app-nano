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

#ifndef NANO_FS_H

#define NANO_FS_H

#include <stdbool.h>

#include "os.h"
#include "nano_context.h"

typedef struct nano_storage_s {
    bool autoReceive;
    bool fidoTransport;
} nano_storage_t;

// the global nvram memory variable
extern nano_storage_t N_nano_real;

#define N_nano (*(nano_storage_t *)PIC(&N_nano_real))

void nano_set_auto_receive(bool enabled);
void nano_set_fido_transport(bool enabled);

#endif
