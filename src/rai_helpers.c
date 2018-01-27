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
#include "rai_apdu_constants.h"

#define RAI_CURVE CX_CURVE_Ed25519

uint32_t rai_read_u32(uint8_t *buffer, bool be,
                      bool skipSign) {
    uint8_t i;
    uint32_t result = 0;
    uint8_t shiftValue = (be ? 24 : 0);
    for (i = 0; i < 4; i++) {
        uint8_t x = (uint8_t)buffer[i];
        if ((i == 0) && skipSign) {
            x &= 0x7f;
        }
        result += ((uint32_t)x) << shiftValue;
        if (be) {
            shiftValue -= 8;
        } else {
            shiftValue += 8;
        }
    }
    return result;
}

void rai_write_u32_be(uint8_t *buffer, uint32_t value) {
    buffer[0] = ((value >> 24) & 0xff);
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = (value & 0xff);
}

void rai_write_u32_le(uint8_t *buffer, uint32_t value) {
    buffer[0] = (value & 0xff);
    buffer[1] = ((value >> 8) & 0xff);
    buffer[2] = ((value >> 16) & 0xff);
    buffer[3] = ((value >> 24) & 0xff);
}

void rai_retrieve_keypair_discard(uint8_t *privateComponent,
                                  bool derivePublic) {
    BEGIN_TRY {
        TRY {
            cx_ecdsa_init_private_key(RAI_CURVE, privateComponent, 32,
                                      &rai_private_key_D);

            L_DEBUG_BUF(("Using private component\n", privateComponent, 32));

            if (derivePublic) {
                cx_ecfp_generate_pair(RAI_CURVE, &rai_public_key_D,
                                      &rai_private_key_D, 1);
            }
        }
        FINALLY {
        }
    }
    END_TRY;
}

void rai_private_derive_keypair(uint8_t *bip32Path,
                                bool derivePublic,
                                uint8_t *out_chainCode) {
    uint8_t bip32PathLength;
    uint8_t i;
    uint32_t bip32PathInt[MAX_BIP32_PATH];
    uint8_t privateComponent[32];

    bip32PathLength = bip32Path[0];
    if (bip32PathLength > MAX_BIP32_PATH) {
        THROW(INVALID_PARAMETER);
    }
    bip32Path++;
    for (i = 0; i < bip32PathLength; i++) {
        bip32PathInt[i] = rai_read_u32(bip32Path, 1, 0);
        bip32Path += 4;
    }
    os_perso_derive_node_bip32(RAI_CURVE, bip32PathInt, bip32PathLength,
                               privateComponent, out_chainCode);
    rai_retrieve_keypair_discard(privateComponent, derivePublic);
    os_memset(privateComponent, 0, sizeof(privateComponent));
}

void rai_swap_bytes(uint8_t *target, uint8_t *source,
                    uint8_t size) {
    uint8_t i;
    for (i = 0; i < size; i++) {
        target[i] = source[size - 1 - i];
    }
}
