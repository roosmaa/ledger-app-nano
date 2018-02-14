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

#ifndef NANO_APDU_GET_ADDRESS_H

#define NANO_APDU_GET_ADDRESS_H

#include "nano_types.h"

typedef struct {
    nano_public_key_t publicKey;
} nano_apdu_get_address_request_t;

uint16_t nano_apdu_get_address(nano_apdu_response_t *resp);

#endif // NANO_APDU_GET_ADDRESS_H
