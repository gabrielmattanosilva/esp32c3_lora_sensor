#include <Arduino.h>
#include <esp_timer.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_system.h> // esp_restart()

#include "pins.h"
#include "utils.h"
#include "pyr20.h"
#include "sx1278_lora.h"

#define TARGET_PERIOD_US (60ULL * 1000000ULL) // 60s

// ---- Variáveis que sobrevivem ao deep sleep ----
RTC_DATA_ATTR uint64_t rtc_total_us = 0;
RTC_DATA_ATTR uint32_t rtc_boot_count = 0;
RTC_DATA_ATTR uint64_t rtc_last_sleep_us = 0;

static const char *TAG_ERR = "HW"; // logs só em erro LoRa

// Em erro grave de LoRa: loga 1 min (a cada 10 s) e reinicia
static void hw_error_restart_lora(const char *msg, int code)
{
  esp_log_level_set("*", ESP_LOG_ERROR);
  for (int i = 0; i < 6; i++)
  { // 6x10s = 60s
    ESP_LOGE(TAG_ERR, "%s (code=%d)", msg, code);
    delay(10000);
  }
  esp_restart();
}

static void go_to_deep_sleep(uint64_t sleep_us)
{
  if (sleep_us < 1000ULL)
    sleep_us = 1000ULL;
  rtc_last_sleep_us = sleep_us;
  lora_sleep();
  esp_sleep_enable_timer_wakeup(sleep_us);
  esp_deep_sleep_start();
}

void setup()
{
  // Operação normal: silencioso
  if (rtc_boot_count > 0)
    rtc_total_us += rtc_last_sleep_us;
  rtc_boot_count++;

  uint64_t t_start_us = esp_timer_get_time();
  pinMode(BATT_VOLT, INPUT);

  // RS485/PYR20
  pyr20_begin();

  // LoRa — se falhar, entra em modo erro+reboot
  if (!lora_begin())
  {
    hw_error_restart_lora("Falha ao iniciar o módulo LoRa (SX1278)", -1);
  }

  // Leitura do PYR20: se falhar, enviar Irr:-1 no payload
  int16_t irr = pyr20_read();
  if (irr < 0)
  {
    irr = -1; // sinaliza falha de RS485 no payload
  }

  float temp_int = utils_read_internal_temp();
  float batt_v = utils_read_battery_voltage();

  uint64_t uptime_now_us = rtc_total_us + (uint64_t)esp_timer_get_time();
  uint32_t uptime_now_s = (uint32_t)(uptime_now_us / 1000000ULL);

  // Payload textual (silencioso na serial quando OK)
  String msg;
  msg.reserve(64);
  msg = "Irr:";
  msg += String(irr); // -1 em caso de falha RS485
  msg += " W/m2,TempESP:";
  msg += String(temp_int, 1);
  msg += " C,Bat:";
  msg += String(batt_v, 2);
  msg += " V,Ts:";
  msg += String(uptime_now_s);
  msg += " s";
  (void)lora_send_string(&msg);

  // Compensação para fechar 60 s
  uint64_t active_us = esp_timer_get_time() - t_start_us;
  rtc_total_us += active_us;
  uint64_t sleep_us = (active_us >= TARGET_PERIOD_US) ? 1000ULL
                                                      : (TARGET_PERIOD_US - active_us);
  go_to_deep_sleep(sleep_us);
}

void loop()
{
  // nunca chega aqui
}
