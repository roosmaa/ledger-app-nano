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
#include "ed25519.h"
#include "blake2b.h"

#define RAI_CURVE CX_CURVE_Ed25519

// Define some binary "literals" for the massive bit manipulation operation
// when converting public key to account string.
#define B_11111 31
#define B_01111 15
#define B_00111  7
#define B_00011  3
#define B_00001  1

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

void rai_private_derive_keypair(uint8_t *bip32Path,
                                bool derivePublic,
                                uint8_t *out_chainCode) {
    uint8_t bip32PathLength;
    uint8_t i;
    uint32_t bip32PathInt[MAX_BIP32_PATH];

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
                               rai_private_key_D, out_chainCode);

    if (derivePublic) {
        ed25519_publickey(rai_private_key_D, rai_public_key_D);
    }
}

void rai_write_account_string(uint8_t *buffer, const uint8_t publicKey[PUBLIC_KEY_LEN]) {
    uint8_t k, i, c;
    uint8_t check[5];

    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(check), NULL, 0);
    blake2b_update(&hash, publicKey, PUBLIC_KEY_LEN);
    blake2b_final(&hash, check);

    // Helper macro to create a virtual array of check and publicKey variables
    #define accountData(x) (uint8_t)( \
        ((x) < sizeof(check)) ? check[(x)] : \
        ((x) - sizeof(check) < PUBLIC_KEY_LEN) ? publicKey[PUBLIC_KEY_LEN - 1 - ((x) - sizeof(check))] : \
        0 \
    )
    for (k = 0; k < ACCOUNT_STRING_LEN - 4; k++) {
        i = (k / 8) * 5;
        c = 0;
        switch (k % 8) {
        case 0:
            c = accountData(i) & B_11111;
            break;
        case 1:
            c = (accountData(i) >> 5) & B_00111;
            c |= (accountData(i + 1) & B_00011) << 3;
            break;
        case 2:
            c = (accountData(i + 1) >> 2) & B_11111;
            break;
        case 3:
            c = (accountData(i + 1) >> 7) & B_00001;
            c |= (accountData(i + 2) & B_01111) << 1;
            break;
        case 4:
            c = (accountData(i + 2) >> 4) & B_01111;
            c |= (accountData(i + 3) & B_00001) << 4;
            break;
        case 5:
            c = (accountData(i + 3) >> 1) & B_11111;
            break;
        case 6:
            c = (accountData(i + 3) >> 6) & B_00011;
            c |= (accountData(i + 4) & B_00111) << 2;
            break;
        case 7:
            c = (accountData(i + 4) >> 3) & B_11111;
            break;
        }
        buffer[ACCOUNT_STRING_LEN-1-k] = BASE32_ALPHABET[c];
    }
    #undef accountData
    buffer[0] = 'x';
    buffer[1] = 'r';
    buffer[2] = 'b';
    buffer[3] = '_';
}