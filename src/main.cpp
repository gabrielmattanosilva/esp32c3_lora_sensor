#include <LoRa.h>
#include <SPI.h>
#include <esp_log.h>
#include <HardwareSerial.h>
#include <esp_sleep.h>
#include <esp_timer.h>

// ----------------- Pinos -----------------
#define ss 7
#define rst 10
#define dio0 2

#define RS485_RX 20
#define RS485_TX 21
#define BATT_VOLT 3

// ----------------- Período de envio -----------------
// Alvo: 60 s entre pacotes (de borda a borda)
#define TARGET_PERIOD_US (60ULL * 1000000ULL)

// ----------------- Log -----------------
static const char *TAG = "LORA_SENSOR";

// ----------------- Serial RS485 -----------------
HardwareSerial modbusSerial(1);

// ---- Variáveis que sobrevivem ao deep sleep ----
RTC_DATA_ATTR uint64_t rtc_total_us = 0;      // tempo total acumulado (ativo + dormindo), em us
RTC_DATA_ATTR uint32_t rtc_boot_count = 0;    // contagem de ciclos
RTC_DATA_ATTR uint64_t rtc_last_sleep_us = 0; // quanto dormimos no ciclo anterior

// ---------- Utilitários ----------
uint8_t lora_read_reg(uint8_t addr)
{
  digitalWrite(ss, LOW);
  SPI.transfer(addr & 0x7F);
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(ss, HIGH);
  return value;
}

uint16_t check_crc(const uint8_t *data, uint8_t length)
{
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++)
    {
      if (crc & 0x0001)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  return crc;
}

static inline void modbus_tx()
{
  static const uint8_t request[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA};
  while (modbusSerial.available())
    (void)modbusSerial.read();
  modbusSerial.write(request, sizeof(request));
  delay(10);
}

static inline int16_t modbus_rx()
{
  uint8_t response[7];
  int i = 0;
  unsigned long startTime = millis();
  while (i < 7 && (millis() - startTime) < 100)
  {
    if (modbusSerial.available())
      response[i++] = modbusSerial.read();
  }
  if (i < 7)
    return -1;

  uint16_t crc_resp = (response[6] << 8) | response[5];
  uint16_t crc_calc = check_crc(response, 5);
  if (crc_resp != crc_calc)
    return -1;

  return (response[3] << 8) | response[4];
}

static inline int16_t readPYR20()
{
  modbus_tx();
  return modbus_rx();
}
static inline float readInternalTemp() { return temperatureRead(); } // °C

static inline float readBatteryVoltageSingle()
{
  int raw = analogRead(BATT_VOLT);
  float v_adc = (raw / 4095.0f) * 3.3f;
  const float DIV_FACTOR = 1.75f; // ajuste para o seu divisor real
  return v_adc * DIV_FACTOR;
}

// ---------- Versões com média de N leituras ----------
static inline int16_t readPYR20_avg(uint8_t n)
{
  long soma = 0;
  uint8_t validas = 0;
  for (uint8_t i = 0; i < n; i++)
  {
    int16_t val = readPYR20();
    if (val >= 0)
    {
      soma += val;
      validas++;
    }
    delay(50); // espaçamento entre leituras Modbus
  }
  if (validas == 0)
    return -1;
  return (int16_t)(soma / validas);
}

static inline float readInternalTemp_avg(uint8_t n)
{
  float soma = 0;
  for (uint8_t i = 0; i < n; i++)
  {
    soma += readInternalTemp();
    delay(10);
  }
  return soma / n;
}

static inline float readBatteryVoltage_avg(uint8_t n)
{
  float soma = 0;
  for (uint8_t i = 0; i < n; i++)
  {
    soma += readBatteryVoltageSingle();
    delay(5);
  }
  return soma / n;
}

// ---------- Sono ----------
static void goToDeepSleep(uint64_t sleep_us)
{
  if (sleep_us < 1000ULL)
    sleep_us = 1000ULL; // mínimo 1 ms de sono

  ESP_LOGI(TAG, "Ativo compensado. Vou dormir por %llu ms",
           (unsigned long long)(sleep_us / 1000ULL));

  // Dormiremos por 'sleep_us' e registramos para somar ao uptime no próximo boot
  rtc_last_sleep_us = sleep_us;

  LoRa.sleep();
  esp_sleep_enable_timer_wakeup(sleep_us);
  Serial.flush();
  esp_deep_sleep_start();
}

// ---------- Fluxo ----------
void setup()
{
  esp_log_level_set("*", ESP_LOG_INFO);
  Serial.begin(115200);
  delay(150);

  // Se não é o primeiro boot, credita o sono anterior ao total
  if (rtc_boot_count > 0)
  {
    rtc_total_us += rtc_last_sleep_us;
  }
  rtc_boot_count++;

  // Marca o início da janela ativa para compensação
  uint64_t t_start_us = esp_timer_get_time();

  // Periféricos
  modbusSerial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  pinMode(BATT_VOLT, INPUT);

  // LoRa
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6))
  {
    ESP_LOGE(TAG, "Falha ao iniciar o módulo LoRa! Indo dormir curto e tentando depois.");
    // tempo ativo até aqui
    uint64_t active_us_fail = esp_timer_get_time() - t_start_us;
    rtc_total_us += active_us_fail;
    // Dorme um trecho do período alvo para tentar de novo
    uint64_t sleep_us_fail = (active_us_fail >= TARGET_PERIOD_US) ? 1000ULL
                                                                  : (TARGET_PERIOD_US - active_us_fail);
    goToDeepSleep(sleep_us_fail);
  }

  pinMode(ss, OUTPUT);
  uint8_t version = lora_read_reg(0x42);
  ESP_LOGI(TAG, "SX1278 RegVersion: 0x%02X", version);
  if (version != 0x12)
  {
    ESP_LOGW(TAG, "RegVersion inesperado. Verifique SPI/módulo.");
  }
  LoRa.setSyncWord(0xA5);

  // ----- Leituras (média de 10) -----
  const uint8_t N = 10;
  int16_t irr = readPYR20_avg(N);
  float temp_int = readInternalTemp_avg(N);
  float batt_v = readBatteryVoltage_avg(N);

  // ----- Timestamp total (incluindo deep sleep) -----
  // total acumulado até agora + tempo ativo decorrido nesta janela
  uint64_t uptime_now_us = rtc_total_us + (uint64_t)esp_timer_get_time();
  uint32_t uptime_now_s = (uint32_t)(uptime_now_us / 1000000ULL);

  // ----- Envio (payload ASCII, formato atual) -----
  if (irr >= 0)
  {
    String msg = "Irr:" + String(irr) +
                 " W/m2,TempESP:" + String(temp_int, 1) +
                 " C,Bat:" + String(batt_v, 2) +
                 " V,Ts:" + String(uptime_now_s) + " s";

    ESP_LOGI(TAG, "Enviando: %s", msg.c_str());
    LoRa.beginPacket();
    LoRa.print(msg);
    LoRa.endPacket();
  }
  else
  {
    ESP_LOGW(TAG, "Falha na leitura do PYR20 (todas as tentativas)");
  }

  // ----- Compensação: calcula tempo ativo e programa sono para fechar 60 s/ciclo -----
  uint64_t active_us = esp_timer_get_time() - t_start_us;
  rtc_total_us += active_us; // credita o tempo ativo da janela ao uptime acumulado

  uint64_t sleep_us = (active_us >= TARGET_PERIOD_US) ? 1000ULL
                                                      : (TARGET_PERIOD_US - active_us);

  // Dorme e registra 'rtc_last_sleep_us' para somar no próximo boot
  goToDeepSleep(sleep_us);
}

void loop()
{
  // nunca chega aqui
}
