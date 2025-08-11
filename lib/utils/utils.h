#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

uint16_t utils_check_crc(const uint8_t *data, uint8_t length);
float utils_read_internal_temp(void);
float utils_read_battery_voltage(void);

#endif /* UTILS_H */
