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

#include "nano_internal.h"
#include "nano_apdu_constants.h"
#include "ed25519.h"
#include "blake2b.h"

#define NANO_CURVE CX_CURVE_Ed25519

// Define some binary "literals" for the massive bit manipulation operation
// when converting public key to account string.
#define B_11111 31
#define B_01111 15
#define B_00111  7
#define B_00011  3
#define B_00001  1

bool nano_is_zero(const uint8_t *ptr, size_t num) {
    while (num > 0) {
        num -= 1;
        if (ptr[num] != 0) {
            return false;
        }
    }
    return true;
}

uint32_t nano_read_u32(uint8_t *buffer, bool be,
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

void nano_write_u32_be(uint8_t *buffer, uint32_t value) {
    buffer[0] = ((value >> 24) & 0xff);
    buffer[1] = ((value >> 16) & 0xff);
    buffer[2] = ((value >> 8) & 0xff);
    buffer[3] = (value & 0xff);
}

void nano_write_u32_le(uint8_t *buffer, uint32_t value) {
    buffer[0] = (value & 0xff);
    buffer[1] = ((value >> 8) & 0xff);
    buffer[2] = ((value >> 16) & 0xff);
    buffer[3] = ((value >> 24) & 0xff);
}

void nano_write_hex_string(uint8_t *buffer, uint8_t *bytes, size_t bytesLen) {
    uint32_t i;
    for (i = 0; i < bytesLen; i++) {
        buffer[2*i] = BASE16_ALPHABET[(bytes[i] >> 4) & 0xF];
        buffer[2*i+1] = BASE16_ALPHABET[bytes[i] & 0xF];
    }
}

bool nano_read_account_string(uint8_t *buffer, size_t size,
                              nano_address_prefix_t *outPrefix,
                              nano_public_key_t outKey) {
    uint8_t k, i, c;
    uint8_t checkInp[5];
    uint8_t check[5];

    // Check prefix and exclude it from the buffer
    if ((buffer[0] == 'n' || buffer[0] == 'N') &&
        (buffer[1] == 'a' || buffer[1] == 'A') &&
        (buffer[2] == 'n' || buffer[2] == 'N') &&
        (buffer[3] == 'o' || buffer[3] == 'O') &&
        (buffer[4] == '-' || buffer[4] == '_')) {
        if (size != NANO_ACCOUNT_STRING_BASE_LEN + NANO_DEFAULT_PREFIX_LEN) {
            return false;
        }
        size -= 5;
        buffer += 5;
        *outPrefix = NANO_DEFAULT_PREFIX;
    } else if ((buffer[0] == 'x' || buffer[0] == 'X') &&
               (buffer[1] == 'r' || buffer[1] == 'R') &&
               (buffer[2] == 'b' || buffer[2] == 'B') &&
               (buffer[3] == '-' || buffer[3] == '_')) {
        if (size != NANO_ACCOUNT_STRING_BASE_LEN + NANO_XRB_PREFIX_LEN) {
            return false;
        }
        size -= 4;
        buffer += 4;
        *outPrefix = NANO_XRB_PREFIX;
    } else {
        return false;
    }

    os_memset(checkInp, 0, sizeof(checkInp));
    os_memset(check, 0, sizeof(check));
    os_memset(outKey, 0, sizeof(nano_public_key_t));

    // Helper macro to create a virtual array of checkInp and outKey variables
    #define accPipeByte(x, v) \
        if ((x) < sizeof(checkInp)) { \
            checkInp[(x)] |= (v);\
        } else if ((x) - sizeof(checkInp) < sizeof(nano_public_key_t)) { \
            outKey[sizeof(nano_public_key_t) - 1 - ((x) - sizeof(checkInp))] |= (v);\
        }
    for (k = 0; k < size; k++) {
        i = (k / 8) * 5;

        c = buffer[size-1-k];
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
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_init(hash, sizeof(check), NULL, 0);
    blake2b_update(hash, outKey, sizeof(nano_public_key_t));
    blake2b_final(hash, check);

    for (i = 0; i < sizeof(check); i++) {
        if (check[i] != checkInp[i]) {
            return false;
        }
    }

    return true;
}

void nano_write_account_string(uint8_t *buffer, nano_address_prefix_t prefix,
                               const nano_public_key_t publicKey) {
    uint8_t k, i, c;
    uint8_t check[5];

    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_init(hash, sizeof(check), NULL, 0);
    blake2b_update(hash, publicKey, sizeof(nano_public_key_t));
    blake2b_final(hash, check);

    switch (prefix) {
    case NANO_DEFAULT_PREFIX:
        buffer[0] = 'n';
        buffer[1] = 'a';
        buffer[2] = 'n';
        buffer[3] = 'o';
        buffer[4] = '_';
        buffer += NANO_DEFAULT_PREFIX_LEN;
        break;
    case NANO_XRB_PREFIX:
        buffer[0] = 'x';
        buffer[1] = 'r';
        buffer[2] = 'b';
        buffer[3] = '_';
        buffer += NANO_XRB_PREFIX_LEN;
        break;
    }

    // Helper macro to create a virtual array of check and publicKey variables
    #define accGetByte(x) (uint8_t)( \
        ((x) < sizeof(check)) ? check[(x)] : \
        ((x) - sizeof(check) < sizeof(nano_public_key_t)) ? publicKey[sizeof(nano_public_key_t) - 1 - ((x) - sizeof(check))] : \
        0 \
    )
    for (k = 0; k < NANO_ACCOUNT_STRING_BASE_LEN; k++) {
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
        buffer[NANO_ACCOUNT_STRING_BASE_LEN-1-k] = BASE32_ALPHABET[c];
    }
    #undef accGetByte
}

int8_t nano_amount_cmp(const nano_amount_t a, const nano_amount_t b) {
    size_t i;
    for (i = 0; i < sizeof(nano_amount_t); i++) {
        if (a[i] != b[i]) {
            return a[i] > b[i] ? 1 : -1;
        }
    }
    return 0;
}

void nano_amount_subtract(nano_amount_t value, const nano_amount_t other) {
    uint8_t mask = -1;
    uint8_t borrow = 0;
    size_t i = sizeof(nano_amount_t);

    while (i > 0) {
        i -= 1;
        uint16_t diff = (uint16_t)value[i] - (other[i] & mask) - borrow;
        value[i] = (uint8_t)diff;
        borrow = -(uint8_t)(diff >> 8);
    }
}

void nano_amount_format(char *dest, size_t destLen,
                        const nano_amount_t balance) {
    nano_amount_format_heap_t *h = &ram_b.nano_amount_format_heap_D;
    os_memset(h->buf, 0, sizeof(h->buf));
    os_memmove(h->num, balance, sizeof(h->num));

    size_t end = sizeof(h->buf);
    end -= 1; // len('\0')
    end -= 5; // len(" Nano")
    size_t start = end;

    // Convert the balance into a string by dividing by 10 until
    // zero is reached
    uint16_t r;
    uint16_t d;
    do {
        r = h->num[0];
        d = r / 10; r = ((r - d * 10) << 8) + h->num[1]; h->num[0] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[2]; h->num[1] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[3]; h->num[2] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[4]; h->num[3] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[5]; h->num[4] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[6]; h->num[5] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[7]; h->num[6] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[8]; h->num[7] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[9]; h->num[8] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[10]; h->num[9] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[11]; h->num[10] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[12]; h->num[11] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[13]; h->num[12] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[14]; h->num[13] = d;
        d = r / 10; r = ((r - d * 10) << 8) + h->num[15]; h->num[14] = d;
        d = r / 10; r = r - d * 10; h->num[15] = d;
        h->buf[--start] = '0' + r;
    } while (h->num[0]  || h->num[1]  || h->num[2]  || h->num[3]  ||
             h->num[4]  || h->num[5]  || h->num[6]  || h->num[7]  ||
             h->num[8]  || h->num[9]  || h->num[10] || h->num[11] ||
             h->num[12] || h->num[13] || h->num[14] || h->num[15]);

    // Assign the location for the decimal point
    size_t point = end - 1 - 30;
    // Make sure that the number is zero padded until the point location
    while (start > point) {
        h->buf[--start] = '0';
    }
    // Move digits before the point one place to the left
    for (size_t i = start; i <= point; i++) {
        h->buf[i-1] = h->buf[i];
    }
    start -= 1;
    // It's safe to write out the point now
    if (point != end) {
        h->buf[point] = '.';
    }

    // Remove as many zeros from the fractional part as possible
    while (end > point && (h->buf[end-1] == '0' || h->buf[end-1] == '.')) {
        end -= 1;
    }
    h->buf[end] = '\0';

    // Append the unit
    h->buf[end++] = ' ';
    h->buf[end++] = 'N';
    h->buf[end++] = 'a';
    h->buf[end++] = 'n';
    h->buf[end++] = 'o';
    h->buf[end] = '\0';

    // Copy the result to the destination buffer
    os_memmove(dest, h->buf + start, MIN(destLen - 1, end - start + 1));
    dest[destLen - 1] = '\0';
}

void nano_derive_keypair(uint8_t *bip32Path,
                         nano_private_key_t out_privateKey,
                         nano_public_key_t out_publicKey) {
    // NB! Explicit scope to limit usage of `h` as it becomes invalid
    //     once the ed25519 function is called. The memory will be
    //     reused for blake2b hashing.
    {
        nano_derive_keypair_heap_t *h = &ram_b.nano_derive_keypair_heap_D;
        uint8_t bip32PathLength;
        uint8_t i;

        bip32PathLength = bip32Path[0];
        if (bip32PathLength > MAX_BIP32_PATH) {
            THROW(INVALID_PARAMETER);
        }
        bip32Path++;
        for (i = 0; i < bip32PathLength; i++) {
            h->bip32PathInt[i] = nano_read_u32(bip32Path, 1, 0);
            bip32Path += 4;
        }
        os_perso_derive_node_bip32(NANO_CURVE, h->bip32PathInt, bip32PathLength,
                                   out_privateKey, h->chainCode);
        os_memset(h->chainCode, 0, sizeof(h->chainCode));
    }

    if (out_publicKey != NULL) {
        ed25519_publickey(out_privateKey, out_publicKey);
    }
}

uint32_t nano_simple_hash(uint8_t *data, size_t dataLen) {
    uint32_t result = 5;
    for (size_t i = 0; i < dataLen; i++) {
        result = 29 * result + data[i];
    }
    return result;
}

void nano_hash_block(nano_hash_t blockHash,
                     const nano_block_data_t *blockData,
                     const nano_public_key_t publicKey) {
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_init(hash, sizeof(nano_hash_t), NULL, 0);

    blake2b_update(hash, BLOCK_HASH_PREAMBLE, sizeof(BLOCK_HASH_PREAMBLE));
    blake2b_update(hash, publicKey, sizeof(nano_public_key_t));
    blake2b_update(hash, blockData->parent, sizeof(blockData->parent));
    blake2b_update(hash, blockData->representative,
        sizeof(blockData->representative));
    blake2b_update(hash, blockData->balance, sizeof(blockData->balance));
    blake2b_update(hash, blockData->link, sizeof(blockData->link));

    blake2b_final(hash, blockHash);
}

void nano_hash_nonce(nano_hash_t nonceHash,
                     const nano_nonce_t nonce) {
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;

    blake2b_init(hash, sizeof(nano_hash_t), NULL, 0);
    blake2b_update(hash, NONCE_HASH_PREAMBLE, sizeof(NONCE_HASH_PREAMBLE));
    blake2b_update(hash, nonce, sizeof(nano_nonce_t));
    blake2b_final(hash, nonceHash);
}

void nano_sign_hash(nano_signature_t signature,
                    const nano_hash_t hash,
                    const nano_private_key_t privateKey,
                    const nano_public_key_t publicKey) {
    ed25519_sign(
        hash, sizeof(nano_hash_t),
        privateKey, publicKey,
        signature);
}

bool nano_verify_hash_signature(const nano_hash_t hash,
                                const nano_public_key_t publicKey,
                                const nano_signature_t signature) {
    return ed25519_sign_open(
        hash, sizeof(nano_hash_t),
        publicKey, signature) == 0;
}
