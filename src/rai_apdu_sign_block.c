/*******************************************************************************
*   RaiBlock Wallet for Ledger Nano S & Blue
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

#include "rai_internal.h"
#include "rai_apdu_constants.h"

#include "rai_bagl.h"

#ifdef HAVE_U2F

#include "u2f_service.h"
#include "u2f_transport.h"

extern bool fidoActivated;
extern volatile u2f_service_t u2fService;
void u2f_proxy_response(u2f_service_t *service, uint16_t tx);

#endif

#define P1_OPEN_BLOCK    0x00
#define P1_RECEIVE_BLOCK 0x01
#define P1_SEND_BLOCK    0x02
#define P1_CHANGE_BLOCK  0x03

#define P2_UNUSED 0x00

uint16_t rai_apdu_sign_block() {
    uint8_t *inPtr;
    uint8_t *outPtr;
    uint8_t keyPath[MAX_BIP32_PATH_LENGTH];
    uint8_t readLen;
    uint8_t chainCode[32];

    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
    case P1_RECEIVE_BLOCK:
    case P1_SEND_BLOCK:
    case P1_CHANGE_BLOCK:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    switch (G_io_apdu_buffer[ISO_OFFSET_P2]) {
    case P2_UNUSED:
        break;
    default:
        return RAI_SW_INCORRECT_P1_P2;
    }

    if (G_io_apdu_buffer[ISO_OFFSET_LC] < 0x01) {
        return RAI_SW_INCORRECT_LENGTH;
    }
    inPtr = G_io_apdu_buffer + ISO_OFFSET_CDATA;
    os_memmove(keyPath, inPtr, MAX_BIP32_PATH_LENGTH);
    inPtr += 1 + (*inPtr) * 4;

    if (!os_global_pin_is_validated()) {
        return RAI_SW_SECURITY_STATUS_NOT_SATISFIED;
    }

    // Derive private & public keys for signing
    rai_private_derive_keypair(keyPath, true, chainCode);

    // Reset block state
    os_memset(&rai_context_D.block, 0, sizeof(rai_context_D.block));

    // Parse input data
    switch (G_io_apdu_buffer[ISO_OFFSET_P1]) {
    case P1_OPEN_BLOCK:
        rai_context_D.block.open.type = RAI_OPEN_BLOCK;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.open.representative)) {
            goto invalidData;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(rai_context_D.block.open.sourceBlock);
        os_memmove(rai_context_D.block.open.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_RECEIVE_BLOCK:
        rai_context_D.block.receive.type = RAI_RECEIVE_BLOCK;

        readLen = sizeof(rai_context_D.block.receive.previousBlock);
        os_memmove(rai_context_D.block.receive.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = sizeof(rai_context_D.block.receive.sourceBlock);
        os_memmove(rai_context_D.block.receive.sourceBlock, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_SEND_BLOCK:
        rai_context_D.block.send.type = RAI_SEND_BLOCK;

        readLen = sizeof(rai_context_D.block.send.previousBlock);
        os_memmove(rai_context_D.block.send.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.send.destinationAccount)) {
            goto invalidData;
        }
        inPtr += 1 + readLen;

        readLen = sizeof(rai_context_D.block.send.balance);
        os_memmove(rai_context_D.block.send.balance, inPtr, readLen);
        inPtr += readLen;
        break;

    case P1_CHANGE_BLOCK:
        rai_context_D.block.change.type = RAI_CHANGE_BLOCK;

        readLen = sizeof(rai_context_D.block.change.previousBlock);
        os_memmove(rai_context_D.block.change.previousBlock, inPtr, readLen);
        inPtr += readLen;

        readLen = *inPtr;
        if (!rai_read_account_string(
                inPtr + 1, readLen,
                rai_context_D.block.change.representative)) {
            goto invalidData;
        }
        inPtr += 1 + readLen;
        break;
    }

    rai_hash_block(&rai_context_D.block);

    // TODO: Prompt user if they wish to sign

    rai_sign_block(&rai_context_D.block);

    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D)); // sanitise private key

    // Begin returning data
    outPtr = G_io_apdu_buffer;

    // Output block hash
    os_memmove(outPtr, rai_context_D.block.base.hash, sizeof(rai_context_D.block.base.hash));
    outPtr += sizeof(rai_context_D.block.base.hash);

    // Output signature
    os_memmove(outPtr, rai_context_D.block.base.signature, sizeof(rai_context_D.block.base.signature));
    outPtr += sizeof(rai_context_D.block.base.signature);

    rai_context_D.outLength = outPtr - G_io_apdu_buffer;

    return RAI_SW_OK;

    invalidData:
    os_memset(rai_private_key_D, 0, sizeof(rai_private_key_D)); // sanitise private key
    return RAI_SW_INCORRECT_DATA;
}