#include "os.h"
#include "ed25519-hash-custom.h"
#include "ed25519-randombytes-custom.h"
#include "blake2b.h"
#include "nano_ram_variables.h"

void ed25519_randombytes_unsafe(void *out, size_t outlen) {
    cx_rng(out, outlen);
}

/* Due to the fact that ed25519-donna uses only one hash at a time,
   we can re-use the blake2b context in shared memory area. */

void ed25519_hash_init(ed25519_hash_context *ctx) {
    UNUSED(ctx);
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_init(hash, 64, NULL, 0);
}

void ed25519_hash_update(ed25519_hash_context *ctx, uint8_t const *in, size_t inlen) {
    UNUSED(ctx);
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_update(hash, in, inlen);
}

void ed25519_hash_final(ed25519_hash_context *ctx, uint8_t *out) {
    UNUSED(ctx);
    blake2b_ctx *hash = &ram_b.blake2b_ctx_D;
    blake2b_final(hash, out);
}

void ed25519_hash (uint8_t *out, uint8_t const *in, size_t inlen) {
    ed25519_hash_context ctx;
    ed25519_hash_init(&ctx);
    ed25519_hash_update(&ctx, in, inlen);
    ed25519_hash_final(&ctx, out);
}
