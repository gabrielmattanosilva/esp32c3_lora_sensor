#include <Arduino.h>
#include <HardwareSerial.h>
#include "pins.h"
#include "pyr20.h"
#include "utils.h"

HardwareSerial rs485Serial(1);

static void pyr20_modbus_tx(void)
{
    static const uint8_t request[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA};

    while (rs485Serial.available())
    {
        (void)rs485Serial.read();
    }

    rs485Serial.write(request, sizeof(request));
    delay(10);
}

static int16_t pyr20_modbus_rx(void)
{
    uint8_t response[7];
    uint8_t i = 0;
    unsigned long startTime = millis();

    while (i < 7 && (millis() - startTime) < 100)
    {
        if (rs485Serial.available())
            response[i++] = rs485Serial.read();
    }
    if (i < 7)
    {
        return -1;
    }

    uint16_t crc_resp = (uint16_t)((response[6] << 8) | response[5]);
    uint16_t crc_calc = utils_check_crc(response, 5);

    if (crc_resp != crc_calc)
    {
        return -1;
    }

    return (int16_t)((response[3] << 8) | response[4]);
}

void pyr20_begin()
{
    rs485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
}

int16_t pyr20_read(void)
{
    pyr20_modbus_tx();
    return pyr20_modbus_rx();
}
