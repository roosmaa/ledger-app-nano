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

#include "rai_public_ram_variables.h"

void rai_set_unit_format(rai_unit_format_t fmt) {
    nvm_write((void *)&N_rai.unitFormat, &fmt,
              sizeof(fmt));
}

void rai_set_auto_receive(bool enabled) {
    nvm_write((void *)&N_rai.autoReceive, &enabled,
              sizeof(enabled));
}

void rai_set_fido_transport(bool enabled) {
    nvm_write((void *)&N_rai.fidoTransport, &enabled,
              sizeof(enabled));
}
