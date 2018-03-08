#include "blake2b.h"
#include "os.h"
#include "cx.h"

int blake2b_init(blake2b_ctx *ctx, size_t outlen,
    const void *key, size_t keylen) {
    cx_blake2b_init(ctx, outlen * 8);
    return 0;
}

void blake2b_update(blake2b_ctx *ctx,
    const void *in, size_t inlen) {
    cx_hash(&ctx->header, 0, (void *)in, inlen, NULL, 0);
}

void blake2b_final(blake2b_ctx *ctx, void *out) {
    cx_hash(&ctx->header, CX_LAST, NULL, 0, out, ctx->ctx.outlen);
}
