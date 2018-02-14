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

#ifndef NANO_APDU_CONSTANTS_H

#define NANO_APDU_CONSTANTS_H

#define NANO_CLA 0xA1
#define NANO_ADM_CLA 0xD0
#define NANO_NFCPAYMENT_CLA 0xF0

#define NANO_INS_GET_APP_CONF 0x01
#define NANO_INS_GET_ADDRESS 0x02
#define NANO_INS_SIGN_BLOCK 0x03

#define NANO_SW_INCORRECT_LENGTH 0x6700
#define NANO_SW_SECURITY_STATUS_NOT_SATISFIED 0x6982
#define NANO_SW_CONDITIONS_OF_USE_NOT_SATISFIED 0x6985
#define NANO_SW_INCORRECT_DATA 0x6A80
#define NANO_SW_INCORRECT_P1_P2 0x6B00
#define NANO_SW_INS_NOT_SUPPORTED 0x6D00
#define NANO_SW_CLA_NOT_SUPPORTED 0x6E00
#define NANO_SW_TECHNICAL_PROBLEM 0x6F00
#define NANO_SW_OK 0x9000
#define NANO_SW_HALTED 0x6FAA
#define NANO_SW_APP_HALTED NANO_SW_CONDITIONS_OF_USE_NOT_SATISFIED

#define ISO_OFFSET_CLA 0x00
#define ISO_OFFSET_INS 0x01
#define ISO_OFFSET_P1 0x02
#define ISO_OFFSET_P2 0x03
#define ISO_OFFSET_LC 0x04
#define ISO_OFFSET_CDATA 0x05

#include "os.h"
#include "nano_secure_value.h"

#endif
