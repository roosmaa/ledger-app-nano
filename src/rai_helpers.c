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

void rai_write_hex_string(uint8_t *buffer, uint8_t *bytes, size_t bytesLen) {
    uint32_t i;
    for (i = 0; i < bytesLen; i++) {
        buffer[2*i] = BASE16_ALPHABET[(bytes[i] >> 4) & 0xF];
        buffer[2*i+1] = BASE16_ALPHABET[bytes[i] & 0xF];
    }
}

bool rai_read_account_string(uint8_t *buffer, size_t size, rai_public_key_t outKey) {
    uint8_t k, i, c;
    uint8_t checkInp[5];
    uint8_t check[5];

    if (size != ACCOUNT_STRING_LEN) {
        return false;
    }
    if (buffer[0] != 'x' || buffer[1] != 'r' || buffer[2] != 'b'
        || !(buffer[3] == '-' || buffer[3] == '_')) {
        return false;
    }

    os_memset(checkInp, 0, sizeof(checkInp));
    os_memset(check, 0, sizeof(check));
    os_memset(outKey, 0, sizeof(rai_public_key_t));

    // Helper macro to create a virtual array of checkInp and outKey variables
    #define accPipeByte(x, v) \
        if ((x) < sizeof(checkInp)) { \
            checkInp[(x)] |= (v);\
        } else if ((x) - sizeof(checkInp) < sizeof(rai_public_key_t)) { \
            outKey[sizeof(rai_public_key_t) - 1 - ((x) - sizeof(checkInp))] |= (v);\
        }
    for (k = 0; k < ACCOUNT_STRING_LEN - 4; k++) {
        i = (k / 8) * 5;

        c = buffer[ACCOUNT_STRING_LEN-1-k];
        if (c >= 0x30 && c < 0x30 + sizeof(BASE32_TABLE)) {
            c = BASE32_TABLE[c - 0x30];
        } else {
            c = 0;
        }

        switch (k % 8) {
        case 0:
            accPipeByte(i, c & B_11111);
            break;
        case 1:
            accPipeByte(i, (c & B_00111) << 5);
            accPipeByte(i + 1, (c >> 3) & B_00011);
            break;
        case 2:
            accPipeByte(i + 1, (c & B_11111) << 2);
            break;
        case 3:
            accPipeByte(i + 1, (c & B_00001) << 7);
            accPipeByte(i + 2, (c >> 1) & B_01111);
            break;
        case 4:
            accPipeByte(i + 2, (c & B_01111) << 4);
            accPipeByte(i + 3, (c >> 4) & B_00001);
            break;
        case 5:
            accPipeByte(i + 3, (c & B_11111) << 1);
            break;
        case 6:
            accPipeByte(i + 3, (c & B_00011) << 6);
            accPipeByte(i + 4, (c >> 2) & B_00111);
            break;
        case 7:
            accPipeByte(i + 4, (c & B_11111) << 3);
            break;
        }
    }
    #undef accPipeByte

    // Verify the checksum of the address
    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(check), NULL, 0);
    blake2b_update(&hash, outKey, sizeof(rai_public_key_t));
    blake2b_final(&hash, check);

    for (i = 0; i < sizeof(check); i++) {
        if (check[i] != checkInp[i]) {
            return false;
        }
    }

    return true;
}

void rai_write_account_string(uint8_t *buffer, const rai_public_key_t publicKey) {
    uint8_t k, i, c;
    uint8_t check[5];

    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(check), NULL, 0);
    blake2b_update(&hash, publicKey, sizeof(rai_public_key_t));
    blake2b_final(&hash, check);

    // Helper macro to create a virtual array of check and publicKey variables
    #define accGetByte(x) (uint8_t)( \
        ((x) < sizeof(check)) ? check[(x)] : \
        ((x) - sizeof(check) < sizeof(rai_public_key_t)) ? publicKey[sizeof(rai_public_key_t) - 1 - ((x) - sizeof(check))] : \
        0 \
    )
    for (k = 0; k < ACCOUNT_STRING_LEN - 4; k++) {
        i = (k / 8) * 5;
        c = 0;
        switch (k % 8) {
        case 0:
            c = accGetByte(i) & B_11111;
            break;
        case 1:
            c = (accGetByte(i) >> 5) & B_00111;
            c |= (accGetByte(i + 1) & B_00011) << 3;
            break;
        case 2:
            c = (accGetByte(i + 1) >> 2) & B_11111;
            break;
        case 3:
            c = (accGetByte(i + 1) >> 7) & B_00001;
            c |= (accGetByte(i + 2) & B_01111) << 1;
            break;
        case 4:
            c = (accGetByte(i + 2) >> 4) & B_01111;
            c |= (accGetByte(i + 3) & B_00001) << 4;
            break;
        case 5:
            c = (accGetByte(i + 3) >> 1) & B_11111;
            break;
        case 6:
            c = (accGetByte(i + 3) >> 6) & B_00011;
            c |= (accGetByte(i + 4) & B_00111) << 2;
            break;
        case 7:
            c = (accGetByte(i + 4) >> 3) & B_11111;
            break;
        }
        buffer[ACCOUNT_STRING_LEN-1-k] = BASE32_ALPHABET[c];
    }
    #undef accGetByte
    buffer[0] = 'x';
    buffer[1] = 'r';
    buffer[2] = 'b';
    buffer[3] = '_';
}

void rai_truncate_string(char *dest, size_t destLen,
                         char *src, size_t srcLen) {
    size_t i;
    os_memset(dest, 0, destLen);
    destLen -= 1; // Leave the \0 c-string terminator

    if (srcLen <= destLen) {
        os_memmove(dest, src, srcLen);
        return;
    } else if (destLen < 9) {
        os_memmove(dest, src, destLen);
        return;
    }

    for (i = 0; i < destLen; i++) {
        if (i < destLen / 2) {
            dest[i] = src[i];
        } else if (i == destLen / 2 || i - 1 == destLen / 2) {
            dest[i] = '.';
        } else {
            dest[i] = src[srcLen - destLen + i];
        }
    }
}

void rai_format_balance(char *dest, size_t destLen,
                        rai_unit_format_t unitFormat,
                        rai_balance_t balance) {
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    char buf[128 / 3 + 1 + 2];
    os_memset(buf, 0, sizeof(buf));
    rai_balance_t num;
    os_memmove(num, balance, sizeof(num));

    size_t end = sizeof(buf) - 1;
    size_t start = end;

    // Convert the balance into a string by dividing by 10 until
    // zero is reached
    uint16_t r;
    uint16_t d;
    do {
        r = num[0];
        d = r / 10; r = ((r - d * 10) << 8) + num[1]; num[0] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[2]; num[1] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[3]; num[2] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[4]; num[3] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[5]; num[4] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[6]; num[5] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[7]; num[6] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[8]; num[7] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[9]; num[8] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[10]; num[9] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[11]; num[10] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[12]; num[11] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[13]; num[12] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[14]; num[13] = d;
        d = r / 10; r = ((r - d * 10) << 8) + num[15]; num[14] = d;
        d = r / 10; r = r - d * 10; num[15] = d;
        buf[--start] = '0' + r;
    } while (num[0]  || num[1]  || num[2]  || num[3]  ||
             num[4]  || num[5]  || num[6]  || num[7]  ||
             num[8]  || num[9]  || num[10] || num[11] ||
             num[12] || num[13] || num[14] || num[15]);

    // Determine the location of the decimal point
    size_t point = end;
    switch (unitFormat) {
    case RAI_MEGA_XRB_UNIT:  point = end - 1 - 30; break;
    case RAI_KILO_XRB_UNIT:  point = end - 1 - 27; break;
    case RAI_XRB_UNIT:       point = end - 1 - 24; break;
    case RAI_MILLI_XRB_UNIT: point = end - 1 - 21; break;
    case RAI_MICRO_XRB_UNIT: point = end - 1 - 18; break;
    }
    // Make sure that the number is zero padded until the point location
    while (start > point) {
        buf[--start] = '0';
    }
    // Move digits before the point one place to the left
    for (size_t i = start; i <= point; i++) {
        buf[i-1] = buf[i];
    }
    start -= 1;
    // It's safe to write out the point now
    if (point != end) {
        buf[point] = '.';
    }

    // Remove as many zeros from the fractional part as possible
    while (end > point && (buf[end-1] == '0' || buf[end-1] == '.')) {
        end -= 1;
    }
    buf[end] = '\0';

    // In case there is still many digits after the decimal point,
    // round it up to 6 digits after the decimal point
    if (end > point + 6 + 1) {
        end = point + 7;
        if (buf[end] >= '5') {
            uint8_t c = 10;
            for (size_t i = end - 1; i >= start; i--) {
                if (buf[i] == '.') continue;
                c = (buf[i] - '0') + c / 10;
                buf[i] = (c % 10) + '0';
                if (c < 10) break;
            }
        }
        buf[end] = '\0';
    }

    // Append the unit
    switch (unitFormat) {
    case RAI_MEGA_XRB_UNIT:
        buf[end++] = ' ';
        buf[end++] = 'M';
        buf[end++] = 'x';
        buf[end++] = 'r';
        buf[end++] = 'b';
        buf[end] = '\0';
        break;
    case RAI_KILO_XRB_UNIT:
        buf[end++] = ' ';
        buf[end++] = 'k';
        buf[end++] = 'x';
        buf[end++] = 'r';
        buf[end++] = 'b';
        buf[end] = '\0';
        break;
    case RAI_XRB_UNIT:
        buf[end++] = ' ';
        buf[end++] = 'x';
        buf[end++] = 'r';
        buf[end++] = 'b';
        buf[end] = '\0';
        break;
    case RAI_MILLI_XRB_UNIT:
        buf[end++] = ' ';
        buf[end++] = 'm';
        buf[end++] = 'x';
        buf[end++] = 'r';
        buf[end++] = 'b';
        buf[end] = '\0';
        break;
    case RAI_MICRO_XRB_UNIT:
        buf[end++] = ' ';
        buf[end++] = 'u';
        buf[end++] = 'x';
        buf[end++] = 'r';
        buf[end++] = 'b';
        buf[end] = '\0';
        break;
    }

    // Copy the result to the destination buffer
    os_memmove(dest, buf + start, MIN(destLen, end - start + 1));
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

void rai_hash_block(rai_block_t *block) {
    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(block->base.hash), NULL, 0);

    switch (block->base.type) {
    case RAI_UNKNOWN_BLOCK: break;
    case RAI_OPEN_BLOCK:
        blake2b_update(&hash, block->open.sourceBlock,
            sizeof(block->open.sourceBlock));
        blake2b_update(&hash, block->open.representative,
            sizeof(block->open.representative));
        blake2b_update(&hash, rai_public_key_D,
            sizeof(rai_public_key_D));
        break;
    case RAI_RECEIVE_BLOCK:
        blake2b_update(&hash, block->receive.previousBlock,
            sizeof(block->receive.previousBlock));
        blake2b_update(&hash, block->receive.sourceBlock,
            sizeof(block->receive.sourceBlock));
        break;
    case RAI_SEND_BLOCK:
        blake2b_update(&hash, block->send.previousBlock,
            sizeof(block->send.previousBlock));
        blake2b_update(&hash, block->send.destinationAccount,
            sizeof(block->send.destinationAccount));
        blake2b_update(&hash, block->send.balance,
            sizeof(block->send.balance));
        break;
    case RAI_CHANGE_BLOCK:
        blake2b_update(&hash, block->change.previousBlock,
            sizeof(block->change.previousBlock));
        blake2b_update(&hash, block->change.representative,
            sizeof(block->change.representative));
        break;
    }

    blake2b_final(&hash, block->base.hash);
}

void rai_sign_block(rai_block_t *block) {
    ed25519_sign(
        block->base.hash, sizeof(block->base.hash),
        rai_private_key_D, rai_public_key_D,
        block->base.signature);
}