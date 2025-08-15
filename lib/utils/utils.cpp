/**
 * @file utils.cpp
 * @brief Implementação de funções utilitárias.
 */

#include "utils.h"
#include <Arduino.h>
#include "pins.h"

#define BATT_VOLT_DIV_FACTOR 1.75f

/****************************** Funções públicas ******************************/

/**
 * @brief Calcula o CRC16 para os dados fornecidos.
 * @param data Ponteiro para os dados.
 * @param length Tamanho dos dados.
 * @return Valor do CRC16 calculado.
 */
uint16_t utils_check_crc(const uint8_t *data, uint8_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief Lê a temperatura interna do ESP32-C3.
 * @return Temperatura em °C*10.
 */
int16_t utils_internal_temp_c10(void)
{
    float tc = temperatureRead();
    int32_t c10 = (int32_t)(tc * 10.0f);

    if (c10 < -32768)
    {
        c10 = -32768;
    }

    if (c10 > 32767)
    {
        c10 = 32767;
    }

    return (int16_t)c10;
}

/**
 * @brief Configura o pino para leitura da tensão da bateria.
 */
void utils_battery_voltage_begin(void)
{
    pinMode(BATT_VOLT, INPUT);
    analogSetPinAttenuation(BATT_VOLT, ADC_11db);
}

/**
 * @brief Lê a tensão da bateria.
 * @return Tensão da bateria em mV.
 */
uint16_t utils_battery_voltage_mv(void)
{
    uint16_t raw = analogRead(BATT_VOLT);
    float v_adc = (raw / 4095.0f) * 3.3f;
    uint32_t mv = (uint32_t)(v_adc * BATT_VOLT_DIV_FACTOR * 1000.0f + 0.5f);

    if (mv > 65535u)
    {
        mv = 65535u;
    }

    return (uint16_t)mv;
}

/**
 * @brief Calcula uma soma de verificação 8-bit para os dados fornecidos.
 * @param data Ponteiro para os dados.
 * @param len Tamanho dos dados.
 * @return Soma de verificação 8-bit.
 */
uint8_t utils_checksum8(const uint8_t *data, uint32_t len)
{
    uint8_t s = 0;

    for (uint32_t i = 0; i < len; ++i)
    {
        s += data[i];
    }

    return s;
}
