#ifndef SX1278_LORA_H
#define SX1278_LORA_H

#include <Arduino.h>
#include <stdbool.h>

bool lora_begin(void);
bool lora_send_string(const String* msg);
void lora_sleep(void);

#endif /* SX1278_LORA_H */
