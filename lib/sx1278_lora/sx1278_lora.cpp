#include "sx1278_lora.h"
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "pins.h"

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

bool lora_send_string(const String *msg)
{
    LoRa.beginPacket();
    LoRa.print(*msg);
    uint8_t rc = LoRa.endPacket();
    return (rc == 1) ? true : false;
}

void lora_sleep(void)
{
    LoRa.sleep();
}
