#pragma once
#include <esp_log.h>

#ifndef LOG_DISABLE
  #define LOGI(TAG, fmt, ...) ESP_LOGI(TAG, fmt, ##__VA_ARGS__)
  #define LOGW(TAG, fmt, ...) ESP_LOGW(TAG, fmt, ##__VA_ARGS__)
  #define LOGE(TAG, fmt, ...) ESP_LOGE(TAG, fmt, ##__VA_ARGS__)
#else
  #define LOGI(TAG, fmt, ...) do{}while(0)
  #define LOGW(TAG, fmt, ...) do{}while(0)
  #define LOGE(TAG, fmt, ...) do{}while(0)
#endif
