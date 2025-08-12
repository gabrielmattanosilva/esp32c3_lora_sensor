/**
 * @file sx1278_lora.h
 * @brief Cabeçalho para as funções de comunicação LoRa com o módulo SX1278.
 */

#ifndef SX1278_LORA_H
#define SX1278_LORA_H

#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Estrutura para o payload compactado enviado via LoRa.
 */
typedef struct __attribute__((packed)) {
    uint16_t irradiance;           /* W/m² (0..2000, 0xFFFF = erro)        */
    uint16_t battery_voltage;      /* mV                                   */
    int16_t  internal_temperature; /* °C ×10                               */
    uint32_t timestamp;            /* s                                    */
    uint8_t  checksum;             /* soma 8-bit (dos 10 bytes anteriores) */
} PayloadPacked;
_Static_assert(sizeof(PayloadPacked) == 11, "Payload deve ter 11 bytes");

bool lora_begin(void);
void lora_sleep(void);
void lora_send_packet(uint16_t irradiance_wm2,
                             uint16_t batt_mv,
                             int16_t  temp_c10,
                             uint32_t timestamp_s);

#endif /* SX1278_LORA_H */
