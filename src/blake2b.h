#ifndef BLAKE2B_H
#define BLAKE2B_H

#include "os.h"
#include "cx.h"

typedef cx_blake2b_t blake2b_ctx;

int blake2b_init(blake2b_ctx *ctx, size_t outlen,
    const void *key, size_t keylen);

void blake2b_update(blake2b_ctx *ctx,
    const void *in, size_t inlen);

void blake2b_final(blake2b_ctx *ctx, void *out);

#endif // BLAKE2B_H
