#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>
#include "pins.h"
#include "sx1278_lora.h"

bool lora_begin(void)
{
    pinMode(SPI_SS, OUTPUT);
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

bool lora_send_string(const String *msg)
{
    LoRa.beginPacket();
    LoRa.print(*msg);
    uint8_t rc = LoRa.endPacket();
    return (rc == 1) ? true : false;
}
