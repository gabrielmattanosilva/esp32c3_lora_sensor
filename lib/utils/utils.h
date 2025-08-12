#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t utils_check_crc(const uint8_t *data, uint8_t length);
int16_t  utils_internal_temp_c10(void);
void utils_battery_voltage_begin(void);
uint16_t utils_battery_voltage_mv(void);
uint8_t  utils_checksum8(const uint8_t *data, uint32_t len);

#endif /* UTILS_H */
