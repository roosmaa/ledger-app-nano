#include "os.h"
#include "ed25519-hash-custom.h"
#include "ed25519-randombytes-custom.h"
#include "blake2b.h"
#include "libn_ram_variables.h"

void ed25519_randombytes_unsafe(void *out, size_t outlen) {
    cx_rng(out, outlen);
}

/* Due to the fact that ed25519-donna uses only one hash at a time,
   we can re-use the blake2b context in shared memory area. */

void ed25519_hash_init(ed25519_hash_context *ctx) {
    blake2b_init(ctx, 64, NULL, 0);
}

void ed25519_hash_update(ed25519_hash_context *ctx, uint8_t const *in, size_t inlen) {
    blake2b_update(ctx, in, inlen);
}

void ed25519_hash_final(ed25519_hash_context *ctx, uint8_t *out) {
    blake2b_final(ctx, out);
}

void ed25519_hash (uint8_t *out, uint8_t const *in, size_t inlen) {
    ed25519_hash_context ctx;
    ed25519_hash_init(&ctx);
    ed25519_hash_update(&ctx, in, inlen);
    ed25519_hash_final(&ctx, out);
}
