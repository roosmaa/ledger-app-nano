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

#ifndef LIBN_BAGL_H

#define LIBN_BAGL_H

#include "libn_app.h"

#define LIBN_BAGL_COLOR_APP 0xFCB653

void libn_bagl_display_address_callback(bool confirmed);
void libn_bagl_confirm_sign_block_callback(bool confirmed);

/** Apply current global state to UX. Returns true if UX was updated,
    false if the UX is already in the correct state and nothing was done. **/
bool libn_bagl_apply_state();

#endif // LIBN_BAGL_H
