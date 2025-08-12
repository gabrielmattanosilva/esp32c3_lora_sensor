#include <Arduino.h>
#include <esp_timer.h>
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_system.h>

#include "pins.h"
#include "utils.h"
#include "pyr20.h"
#include "sx1278_lora.h"

#define SLEEP_PERIOD_MIN 1ULL
#define TARGET_PERIOD_US (SLEEP_PERIOD_MIN * 60ULL * 1000000ULL)

RTC_DATA_ATTR uint64_t rtc_total_us = 0;
RTC_DATA_ATTR uint32_t rtc_boot_count = 0;
RTC_DATA_ATTR uint64_t rtc_last_sleep_us = 0;

static void hw_error_restart_lora(const char *msg, int code)
{
    esp_log_level_set("*", ESP_LOG_ERROR);
    for (uint8_t i = 0; i < 6; i++)
    {
        ESP_LOGE("HW", "%s (code=%d)", msg, code);
        delay(10000);
    }
    esp_restart();
}

static void go_to_deep_sleep(uint64_t sleep_us)
{
    if (sleep_us < 1000ULL)
    {
        sleep_us = 1000ULL;
    }

    rtc_last_sleep_us = sleep_us;
    lora_sleep();
    esp_sleep_enable_timer_wakeup(sleep_us);
    esp_deep_sleep_start();
}

void setup()
{
    if (rtc_boot_count > 0)
    {
        rtc_total_us += rtc_last_sleep_us;
    }

    rtc_boot_count++;
    uint64_t t_start_us = esp_timer_get_time();

    utils_read_battery_voltage_begin();
    pyr20_begin();
    if (!lora_begin())
    {
        hw_error_restart_lora("Falha ao iniciar o módulo LoRa (SX1278)", -1);
    }

    int16_t irr = pyr20_read();
    float temp_int = utils_read_internal_temp();
    float batt_v = utils_read_battery_voltage();
    uint64_t uptime_now_us = rtc_total_us + (uint64_t)esp_timer_get_time();
    uint32_t uptime_now_s = (uint32_t)(uptime_now_us / 1000000ULL);

    String msg;
    msg.reserve(64);
    msg = "Irr:";
    msg += String(irr);
    msg += " W/m2,TempESP:";
    msg += String(temp_int, 1);
    msg += " C,Bat:";
    msg += String(batt_v, 2);
    msg += " V,Ts:";
    msg += String(uptime_now_s);
    msg += " s";
    (void)lora_send_string(&msg);

    uint64_t active_us = esp_timer_get_time() - t_start_us;
    rtc_total_us += active_us;
    uint64_t sleep_us = (active_us >= TARGET_PERIOD_US) ? 1000ULL : (TARGET_PERIOD_US - active_us);
    go_to_deep_sleep(sleep_us);
}

void loop()
{
}
