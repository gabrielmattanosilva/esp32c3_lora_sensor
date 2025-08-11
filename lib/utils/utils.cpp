#include <Arduino.h>
#include "utils.h"
#include "pins.h"

#define DIV_FACTOR 1.75

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

float utils_read_internal_temp(void)
{
    return temperatureRead();
}

float utils_read_battery_voltage(void)
{
    uint16_t raw = analogRead(BATT_VOLT);
    float v_adc = (raw / 4095.0f) * 3.3f;
    return v_adc * DIV_FACTOR;
}
