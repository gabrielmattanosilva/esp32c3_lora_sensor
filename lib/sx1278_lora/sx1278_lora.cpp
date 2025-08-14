/**
 * @file sx1278_lora.cpp
 * @brief Implementação das funções para comunicação LoRa com o módulo SX1278.
 */

#include "sx1278_lora.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "pins.h"
#include "utils.h"
#include "crypto.h"
#include "credentials.h"

/**
 * @brief Inicializa o módulo LoRa.
 * @return Verdadeiro se a inicialização foi bem-sucedida, falso caso contrário.
 */
bool lora_begin(void)
{
    pinMode(SPI_SS, OUTPUT);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS);
    LoRa.setSPI(SPI);
    LoRa.setPins(SPI_SS, SX1278_RST, SX1278_DIO0);

    if (!LoRa.begin(433E6))
    {
        return false;
    }

    LoRa.setSyncWord(0xA5);

    crypto_init(AES_KEY);
    return true;
}

/**
 * @brief Coloca o módulo LoRa em modo de baixo consumo.
 */
void lora_sleep(void)
{
    LoRa.sleep();
}

/**
 * @brief Envia um pacote de dados via LoRa.
 * @param irradiance_wm2 Valor de irradiância em W/m^2.
 * @param batt_mv Tensão da bateria em mV.
 * @param temp_c10 Temperatura interna em °C*10.
 * @param timestamp_s Timestamp em segundos.
 */
void lora_send_packet(uint16_t irradiance_wm2,
                      uint16_t batt_mv,
                      int16_t  temp_c10,
                      uint32_t timestamp_s)
{
    PayloadPacked p;
    p.irradiance            = irradiance_wm2;
    p.battery_voltage       = batt_mv;
    p.internal_temperature  = temp_c10;
    p.timestamp             = timestamp_s;
    p.checksum              = utils_checksum8((const uint8_t *)&p, sizeof(PayloadPacked) - 1);

    uint8_t iv[CRYPTO_BLOCK_SIZE];
    crypto_random_iv(iv);

    uint8_t ct[CRYPTO_BLOCK_SIZE];
    size_t  ct_len = 0;
    (void)crypto_encrypt((const uint8_t *)&p, sizeof(PayloadPacked), iv, ct, &ct_len);

    LoRa.beginPacket();
    LoRa.write(iv, CRYPTO_BLOCK_SIZE);
    LoRa.write(ct, ct_len);
    (void)LoRa.endPacket();
}
