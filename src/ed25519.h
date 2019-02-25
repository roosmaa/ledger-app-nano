#ifndef ED25519_H
#define ED25519_H

#include <stddef.h>
#include "libn_types.h"

void ed25519_publickey(const libn_private_key_t privateKey,
                       libn_public_key_t publicKey);

void ed25519_sign(const uint8_t *m, size_t mlen,
                  const libn_private_key_t privateKey,
                  const libn_public_key_t publicKey,
                  libn_signature_t signature);

int ed25519_sign_open(const uint8_t *m, size_t mlen,
                      const libn_public_key_t publicKey,
                      const libn_signature_t signature);

#endif // ED25519_H
