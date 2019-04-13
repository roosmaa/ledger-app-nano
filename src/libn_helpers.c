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

#include "coins.h"
#include "libn_internal.h"
#include "libn_apdu_constants.h"
#include "ed25519.h"
#include "blake2b.h"

#define LIBN_CURVE CX_CURVE_Ed25519

// Define some binary "literals" for the massive bit manipulation operation
// when converting public key to account string.
#define B_11111 31
#define B_01111 15
#define B_00111  7
#define B_00011  3
#define B_00001  1

bool libn_is_zero(const uint8_t *ptr, size_t num) {
    while (num > 0) {
        num -= 1;
        if (ptr[num] != 0) {
            return false;
        }
    }
    return true;
}

uint32_t libn_read_u32(uint8_t *buffer, bool be,
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

void libn_write_u32_be(uint8_t *buffer, uint32_t value) {
    buffer[0] = ((value >> 24) & 0xff);
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = (value & 0xff);
}

void libn_write_u32_le(uint8_t *buffer, uint32_t value) {
    buffer[0] = (value & 0xff);
    buffer[1] = ((value >> 8) & 0xff);
    buffer[2] = ((value >> 16) & 0xff);
    buffer[3] = ((value >> 24) & 0xff);
}

void libn_write_hex_string(uint8_t *buffer, const uint8_t *bytes, size_t bytesLen) {
    uint32_t i;
    for (i = 0; i < bytesLen; i++) {
        buffer[2*i] = BASE16_ALPHABET[(bytes[i] >> 4) & 0xF];
        buffer[2*i+1] = BASE16_ALPHABET[bytes[i] & 0xF];
    }
}

// Extract a single-path component from the encoded bip32 path
uint32_t libn_bip32_get_component(uint8_t *path, uint8_t n) {
    uint8_t pathLength = path[0];
    if (n >= pathLength) {
        THROW(INVALID_PARAMETER);
    }
    return libn_read_u32(&path[1 + 4 * n], 1, 0);
}

void libn_address_formatter_for_coin(libn_address_formatter_t *fmt, libn_address_prefix_t prefix, uint8_t *bip32Path) {
    // NOS multi-currency coin handling
    if (strcmp(COIN_NAME, "NOS") == 0) {
        uint32_t currencyComponent = libn_bip32_get_component(bip32Path, 2);

        #define SUBCURRENCY(CODE, PREFIX, SUFFIX, SCALE) \
            case HARDENED((CODE)): \
                fmt->prefix = (PREFIX); \
                fmt->prefixLen = strlen((PREFIX)); \
                break
        switch (currencyComponent) {
        #include "nos_subcurrencies.h"
        case HARDENED(0):
            fmt->prefix = COIN_PRIMARY_PREFIX;
            fmt->prefixLen = strnlen(COIN_PRIMARY_PREFIX, sizeof(COIN_PRIMARY_PREFIX));
            break;
        default:
            THROW(INVALID_PARAMETER);
        }
        #undef SUBCURRENCY
        return;
    }

    // Default prefixes
    switch (prefix) {
    case LIBN_PRIMARY_PREFIX:
        fmt->prefix = COIN_PRIMARY_PREFIX;
        fmt->prefixLen = strnlen(COIN_PRIMARY_PREFIX, sizeof(COIN_PRIMARY_PREFIX));
        break;
    case LIBN_SECONDARY_PREFIX:
        fmt->prefix = COIN_SECONDARY_PREFIX;
        fmt->prefixLen = strnlen(COIN_SECONDARY_PREFIX, sizeof(COIN_SECONDARY_PREFIX));
        break;
    }
}

void libn_amount_formatter_for_coin(libn_amount_formatter_t *fmt, uint8_t *bip32Path) {
    // NOS multi-currency coin handling
    if (strcmp(COIN_NAME, "NOS") == 0) {
        uint32_t currencyComponent = libn_bip32_get_component(bip32Path, 2);

        #define SUBCURRENCY(CODE, PREFIX, SUFFIX, SCALE) \
            case HARDENED((CODE)): \
                fmt->suffix = (SUFFIX); \
                fmt->suffixLen = strlen((SUFFIX)); \
                fmt->unitScale = SCALE; \
                break
        switch (currencyComponent) {
        #include "nos_subcurrencies.h"
        case HARDENED(0):
            fmt->suffix = COIN_UNIT;
            fmt->suffixLen = strnlen(COIN_UNIT, sizeof(COIN_UNIT));
            fmt->unitScale = COIN_UNIT_SCALE;
            break;
        default:
            THROW(INVALID_PARAMETER);
        }
        #undef SUBCURRENCY
        return;
    }

    // Default prefixes
    fmt->suffix = COIN_UNIT;
    fmt->suffixLen = strnlen(COIN_UNIT, sizeof(COIN_UNIT));
    fmt->unitScale = COIN_UNIT_SCALE;
}

size_t libn_address_format(const libn_address_formatter_t *fmt,
                           uint8_t *buffer,
                           const libn_public_key_t publicKey) {
    uint8_t k, i, c;
    uint8_t check[5];

    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(check), NULL, 0);
    blake2b_update(&hash, publicKey, sizeof(libn_public_key_t));
    blake2b_final(&hash, check);

    // Write prefix
    os_memmove(buffer, fmt->prefix, fmt->prefixLen);
    buffer += fmt->prefixLen;

    // Helper macro to create a virtual array of check and publicKey variables
    #define accGetByte(x) (uint8_t)( \
        ((x) < sizeof(check)) ? check[(x)] : \
        ((x) - sizeof(check) < sizeof(libn_public_key_t)) ? publicKey[sizeof(libn_public_key_t) - 1 - ((x) - sizeof(check))] : \
        0 \
    )
    for (k = 0; k < LIBN_ACCOUNT_STRING_BASE_LEN; k++) {
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
        buffer[LIBN_ACCOUNT_STRING_BASE_LEN-1-k] = BASE32_ALPHABET[c];
    }
    #undef accGetByte
    return fmt->prefixLen + LIBN_ACCOUNT_STRING_BASE_LEN;
}

int8_t libn_amount_cmp(const libn_amount_t a, const libn_amount_t b) {
    size_t i;
    for (i = 0; i < sizeof(libn_amount_t); i++) {
        if (a[i] != b[i]) {
            return a[i] > b[i] ? 1 : -1;
        }
    }
    return 0;
}

void libn_amount_subtract(libn_amount_t value, const libn_amount_t other) {
    uint8_t mask = -1;
    uint8_t borrow = 0;
    size_t i = sizeof(libn_amount_t);

    while (i > 0) {
        i -= 1;
        uint16_t diff = (uint16_t)value[i] - (other[i] & mask) - borrow;
        value[i] = (uint8_t)diff;
        borrow = -(uint8_t)(diff >> 8);
    }
}

void libn_amount_format(const libn_amount_formatter_t *fmt,
                        char *dest, size_t destLen,
                        const libn_amount_t balance) {
    // MaxUInt128 = 340282366920938463463374607431768211455 (39 digits)
    char buf[39 /* digits */
             + 1 /* period */
             + 1 /* " " before unit */
             + sizeof((libn_coin_conf_t){}.defaultUnit)
             + 1 /* '\0' NULL terminator */];
    libn_amount_t num;

    os_memset(buf, 0, sizeof(buf));
    os_memmove(num, balance, sizeof(num));

    size_t end = sizeof(buf);
    end -= 1; // '\0' NULL terminator
    end -= 1 + fmt->suffixLen; // len(" " + suffix)
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

    // Assign the location for the decimal point
    size_t point = end - 1 - fmt->unitScale;
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

    // Append the unit
    buf[end++] = ' ';
    os_memmove(buf + end, fmt->suffix, fmt->suffixLen);
    end += fmt->suffixLen;
    buf[end] = '\0';

    // Copy the result to the destination buffer
    os_memmove(dest, buf + start, MIN(destLen - 1, end - start + 1));
    dest[destLen - 1] = '\0';
}

void libn_derive_keypair(uint8_t *bip32Path,
                         libn_private_key_t out_privateKey,
                         libn_public_key_t out_publicKey) {
    uint32_t bip32PathInt[MAX_BIP32_PATH];
    uint8_t chainCode[32];
    uint8_t bip32PathLength;
    uint8_t i;
    const uint8_t bip32PrefixLength = 2;

    bip32PathLength = bip32Path[0];
    if (bip32PathLength > MAX_BIP32_PATH) {
        THROW(INVALID_PARAMETER);
    }
    bip32Path++;
    for (i = 0; i < bip32PathLength; i++) {
        bip32PathInt[i] = libn_read_u32(bip32Path, 1, 0);
        bip32Path += 4;
    }
    // Verify that the prefix is the allowed prefix
    if (bip32PathLength < bip32PrefixLength) {
        THROW(INVALID_PARAMETER);
    }
    for (i = 0; i < bip32PrefixLength; i++) {
        if (bip32PathInt[i] != COIN_BIP32_PREFIX[i]) {
            THROW(INVALID_PARAMETER);
        }
    }
    os_perso_derive_node_bip32(LIBN_CURVE, bip32PathInt, bip32PathLength,
                               out_privateKey, chainCode);
    os_memset(chainCode, 0, sizeof(chainCode));

    if (out_publicKey != NULL) {
        ed25519_publickey(out_privateKey, out_publicKey);
    }
}

uint32_t libn_simple_hash(uint8_t *data, size_t dataLen) {
    uint32_t result = 5;
    for (size_t i = 0; i < dataLen; i++) {
        result = 29 * result + data[i];
    }
    return result;
}

void libn_hash_block(libn_hash_t blockHash,
                     const libn_block_data_t *blockData,
                     const libn_public_key_t publicKey) {
    blake2b_ctx hash;
    blake2b_init(&hash, sizeof(libn_hash_t), NULL, 0);

    blake2b_update(&hash, BLOCK_HASH_PREAMBLE, sizeof(BLOCK_HASH_PREAMBLE));
    blake2b_update(&hash, publicKey, sizeof(libn_public_key_t));
    blake2b_update(&hash, blockData->parent, sizeof(blockData->parent));
    blake2b_update(&hash, blockData->representative,
        sizeof(blockData->representative));
    blake2b_update(&hash, blockData->balance, sizeof(blockData->balance));
    blake2b_update(&hash, blockData->link, sizeof(blockData->link));

    blake2b_final(&hash, blockHash);
}

void libn_sign_hash(libn_signature_t signature,
                    const libn_hash_t hash,
                    const libn_private_key_t privateKey,
                    const libn_public_key_t publicKey) {
    ed25519_sign(
        hash, sizeof(libn_hash_t),
        privateKey, publicKey,
        signature);
}

bool libn_verify_hash_signature(const libn_hash_t hash,
                                const libn_public_key_t publicKey,
                                const libn_signature_t signature) {
    return ed25519_sign_open(
        hash, sizeof(libn_hash_t),
        publicKey, signature) == 0;
}

void libn_sign_nonce(libn_signature_t signature,
                     const libn_nonce_t nonce,
                     const libn_private_key_t privateKey,
                     const libn_public_key_t publicKey) {
    uint8_t msg[sizeof(COIN_NAME) +
                sizeof(NONCE_PREAMBLE) +
                2 * sizeof(libn_nonce_t)];
    size_t len;

    // Construct the message to contain the preamble with the hex-encoded
    // nonce (eg. "Nano Signed Nonce:\n96DFEF63B836C9B1DD57CFF76F6DF3D0")
    uint8_t *ptr = msg;
    // Append the coin name
    len = strnlen(COIN_NAME, sizeof(COIN_NAME));
    os_memmove(ptr, COIN_NAME, len);
    ptr += len;
    // Apend the " Signed Nonce:\n"
    os_memmove(ptr, NONCE_PREAMBLE, sizeof(NONCE_PREAMBLE));
    ptr += sizeof(NONCE_PREAMBLE);
    // Append the nonce
    libn_write_hex_string(ptr, nonce, sizeof(libn_nonce_t));
    ptr += 2 * sizeof(libn_nonce_t);

    len = ptr - msg;
    ed25519_sign(
        msg, len,
        privateKey, publicKey,
        signature);
}
