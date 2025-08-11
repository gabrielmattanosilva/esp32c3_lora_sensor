#ifndef SX1278_LORA_H
#define SX1278_LORA_H

#include <stdbool.h>
#include <stdint.h>

void lora_sleep(void);
bool lora_begin(void);
bool lora_send_string(const String* msg);

#endif /* SX1278_LORA_H */
