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

#ifndef RAI_FS_H

#define RAI_FS_H

#include <stdbool.h>

#include "os.h"
#include "rai_context.h"

typedef enum {
    RAI_MEGA_XRB_UNIT,
    RAI_KILO_XRB_UNIT,
    RAI_XRB_UNIT,
    RAI_MILLI_XRB_UNIT,
    RAI_MICRO_XRB_UNIT,
} rai_unit_format_t;

typedef struct rai_storage_s {
    rai_unit_format_t unitFormat;
    bool autoReceive;
    bool fidoTransport;
} rai_storage_t;

// the global nvram memory variable
extern rai_storage_t N_rai_real;

#define N_rai (*(rai_storage_t *)PIC(&N_rai_real))

void rai_set_unit_format(rai_unit_format_t fmt);
void rai_set_auto_receive(bool enabled);
void rai_set_fido_transport(bool enabled);

#endif
