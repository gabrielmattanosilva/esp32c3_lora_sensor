/**
 * @file pyr20.cpp
 * @brief Implementação das funções para comunicação com o piranômetro PYR20.
 */

#include "pyr20.h"
#include <Arduino.h>
#include <HardwareSerial.h>
#include "pins.h"
#include "utils.h"

HardwareSerial modbusSerial(1);

/****************************** Funções privadas ******************************/

/**
 * @brief Envia uma requisição Modbus para o piranômetro PYR20.
 */
static void modbus_tx(void)
{
    static const uint8_t request[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA};

    while (modbusSerial.available())
    {
        (void)modbusSerial.read();
    }

    modbusSerial.write(request, sizeof(request));
    delay(10);
}

/**
 * @brief Recebe e processa a resposta do piranômetro PYR20.
 * @return Valor lido do piranômetro ou 0xFFFF em caso de erro.
 */
static uint16_t modbus_rx(void)
{
    uint8_t response[7];
    uint8_t i = 0;
    uint32_t t0 = millis();

    while (i < 7 && (millis() - t0) < 100)
    {
        if (modbusSerial.available())
            response[i++] = modbusSerial.read();
    }

    if (i < 7)
    {
        return 0xFFFFu;
    }

    uint16_t crc_resp = (uint16_t)((response[6] << 8) | response[5]);
    uint16_t crc_calc = utils_check_crc(response, 5);

    if (crc_resp != crc_calc)
    {
        return 0xFFFFu;
    }

    uint16_t val = (uint16_t)((response[3] << 8) | response[4]);
    return val;
}

/****************************** Funções públicas ******************************/

/**
 * @brief Inicializa a comunicação com o piranômetro PYR20.
 */
void pyr20_begin()
{
    modbusSerial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
}

/**
 * @brief Lê o valor de irradiância do piranômetro PYR20.
 * @return Valor de irradiância em W/m^2 ou 0xFFFF em caso de erro.
 */
uint16_t pyr20_read(void)
{
    modbus_tx();
    return modbus_rx();
}
