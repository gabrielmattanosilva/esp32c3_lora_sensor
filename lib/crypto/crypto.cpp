#include "crypto.h"
#include <string.h>
#include <mbedtls/aes.h>
#include <esp_random.h>

static uint8_t g_key[CRYPTO_KEY_SIZE];

void crypto_init(const uint8_t *key16)
{
    memcpy(g_key, key16, CRYPTO_KEY_SIZE);
}

void crypto_random_iv(uint8_t iv[CRYPTO_BLOCK_SIZE])
{
    for (uint8_t i = 0; i < CRYPTO_BLOCK_SIZE; i += 4)
    {
        uint32_t r = esp_random();
        memcpy(&iv[i], &r, 4);
    }
}

static inline size_t pkcs7_pad(const uint8_t *in, size_t in_len, uint8_t *out)
{
    const uint8_t pad = (uint8_t)(CRYPTO_BLOCK_SIZE - (in_len % CRYPTO_BLOCK_SIZE ? (in_len % CRYPTO_BLOCK_SIZE) : CRYPTO_BLOCK_SIZE));
    const size_t out_len = in_len + pad;
    memcpy(out, in, in_len);
    memset(out + in_len, pad, pad);
    return out_len;
}

static inline bool pkcs7_unpad(uint8_t *buf, size_t in_len, size_t *out_len)
{
    if (in_len == 0 || (in_len % CRYPTO_BLOCK_SIZE) != 0)
    {
        return false;
    }

    uint8_t pad = buf[in_len - 1];

    if (pad == 0 || pad > CRYPTO_BLOCK_SIZE)
    {
        return false;
    }

    for (size_t i = 0; i < pad; ++i)
    {
        if (buf[in_len - 1 - i] != pad)
        {
            return false;
        }
    }

    *out_len = in_len - pad;
    return true;
}

bool crypto_encrypt_cbc(const uint8_t *in, size_t in_len,
                        const uint8_t iv[CRYPTO_BLOCK_SIZE],
                        uint8_t *out, size_t *out_len)
{
    const size_t padded_len = pkcs7_pad(in, in_len, out);

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

bool crypto_decrypt_cbc(const uint8_t *in, size_t in_len,
                        const uint8_t iv[CRYPTO_BLOCK_SIZE],
                        uint8_t *out, size_t *out_len)
{
    if ((in_len % CRYPTO_BLOCK_SIZE) != 0)
    {
        return false;
    }

    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);

    if (mbedtls_aes_setkey_dec(&ctx, g_key, 128) != 0)
    {
        mbedtls_aes_free(&ctx);
        return false;
    }

    uint8_t iv_copy[CRYPTO_BLOCK_SIZE];
    memcpy(iv_copy, iv, CRYPTO_BLOCK_SIZE);

    int rc = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, in_len, iv_copy, in, out);
    mbedtls_aes_free(&ctx);

    if (rc != 0)
    {
        return false;
    }

    return pkcs7_unpad(out, in_len, out_len);
}
