/**
 * @file crypto.cpp
 * @brief Implementação das funções de criptografia AES-128-CBC.
 */

#include "crypto.h"
#include <string.h>
#include <mbedtls/aes.h>
#include <esp_random.h>

static uint8_t g_key[CRYPTO_KEY_SIZE];

/**
 * @brief Inicializa o módulo de criptografia com a chave fornecida.
 * @param key16 Ponteiro para a chave AES de 16 bytes.
 */
void crypto_init(const uint8_t *key16)
{
    memcpy(g_key, key16, CRYPTO_KEY_SIZE);
}

/**
 * @brief Gera um vetor de inicialização (IV) aleatório.
 * @param iv Array para armazenar o IV gerado.
 */
void crypto_random_iv(uint8_t iv[CRYPTO_BLOCK_SIZE])
{
    for (uint8_t i = 0; i < CRYPTO_BLOCK_SIZE; i += 4)
    {
        uint32_t r = esp_random();
        memcpy(&iv[i], &r, 4);
    }
}

/**
 * @brief Aplica padding PKCS#7 aos dados de entrada.
 * @param in Dados de entrada.
 * @param in_len Tamanho dos dados de entrada.
 * @param out Buffer para os dados com padding.
 * @return Tamanho dos dados após o padding.
 */
static inline size_t crypto_pkcs7_pad(const uint8_t *in, size_t in_len, uint8_t *out)
{
    const uint8_t pad = (uint8_t)(CRYPTO_BLOCK_SIZE - (in_len % CRYPTO_BLOCK_SIZE ? (in_len % CRYPTO_BLOCK_SIZE) : CRYPTO_BLOCK_SIZE));
    const size_t out_len = in_len + pad;
    memcpy(out, in, in_len);
    memset(out + in_len, pad, pad);
    return out_len;
}

/**
 * @brief Criptografa dados usando AES-128-CBC.
 * @param in Dados de entrada.
 * @param in_len Tamanho dos dados de entrada.
 * @param iv Vetor de inicialização.
 * @param out Buffer para os dados criptografados.
 * @param out_len Ponteiro para armazenar o tamanho dos dados criptografados.
 * @return Verdadeiro se a criptografia foi bem-sucedida, falso caso contrário.
 */
bool crypto_encrypt(const uint8_t *in, size_t in_len,
                        const uint8_t iv[CRYPTO_BLOCK_SIZE],
                        uint8_t *out, size_t *out_len)
{
    const size_t padded_len = crypto_pkcs7_pad(in, in_len, out);

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    if (mbedtls_aes_setkey_enc(&ctx, g_key, 128) != 0)
    {
        mbedtls_aes_free(&ctx);
        return false;
    }

    uint8_t iv_copy[CRYPTO_BLOCK_SIZE];
    memcpy(iv_copy, iv, CRYPTO_BLOCK_SIZE);

    int rc = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, padded_len, iv_copy, out, out);
    mbedtls_aes_free(&ctx);

    if (rc != 0)
    {
        return false;
    }

    *out_len = padded_len;
    return true;
}
