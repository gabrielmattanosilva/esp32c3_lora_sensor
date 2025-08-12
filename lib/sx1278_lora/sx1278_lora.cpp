#include "sx1278_lora.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "pins.h"
#include "utils.h"

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
    return true;
}

void lora_sleep(void)
{
    LoRa.sleep();
}

void lora_send_packet(uint16_t irradiance_wm2,
                      uint16_t batt_mv,
                      int16_t temp_c10,
                      uint32_t timestamp_s)
{
    PayloadPacked p;
    p.irradiance = irradiance_wm2;
    p.battery_voltage = batt_mv;
    p.internal_temperature = temp_c10;
    p.timestamp = timestamp_s;
    p.checksum = utils_checksum8((const uint8_t *)&p, sizeof(PayloadPacked) - 1);

    LoRa.beginPacket();
    LoRa.write((const uint8_t *)&p, sizeof(PayloadPacked));
    (void)LoRa.endPacket();
}
