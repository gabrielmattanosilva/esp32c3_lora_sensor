/**
 * @file crypto.h
 * @brief Cabeçalho para as funções de criptografia AES-CBC.
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#define CRYPTO_KEY_SIZE   16
#define CRYPTO_BLOCK_SIZE 16

void crypto_init(const uint8_t *key16);
void crypto_random_iv(uint8_t iv[CRYPTO_BLOCK_SIZE]);
bool crypto_encrypt_cbc(const uint8_t *in, size_t in_len,
                        const uint8_t iv[CRYPTO_BLOCK_SIZE],
                        uint8_t *out, size_t *out_len);
bool crypto_decrypt_cbc(const uint8_t *in, size_t in_len,
                        const uint8_t iv[CRYPTO_BLOCK_SIZE],
                        uint8_t *out, size_t *out_len);

#endif // CRYPTO_H
