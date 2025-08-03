#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>
#include <esp_log.h>
#include <Adafruit_AHTX0.h>
#include <HardwareSerial.h>

#define ss    7
#define rst   10
#define dio0  2

#define I2C_SDA 0
#define I2C_SCL 1

#define RS485_RX 20
#define RS485_TX 21

static const char *TAG = "LORA_AHTX0";

Adafruit_AHTX0 aht;
HardwareSerial modbusSerial(1);

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;

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

void modbus_tx()
{
  static const uint8_t request[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xCA};
  while (modbusSerial.available())
    modbusSerial.read();
  modbusSerial.write(request, sizeof(request));
  delay(10);
}

int16_t modbus_rx()
{
  uint8_t response[7];
  int i = 0;
  unsigned long startTime = millis();
  while (i < 7 && millis() - startTime < 100)
  {
    if (modbusSerial.available())
    {
      response[i++] = modbusSerial.read();
    }
  }

  if (i < 7)
    return -1;

  uint16_t crc_resp = (response[6] << 8) | response[5];
  uint16_t crc_calc = check_crc(response, 5);
  if (crc_resp != crc_calc)
    return -1;

  return (response[3] << 8) | response[4];
}

int16_t readPYR20()
{
  modbus_tx();
  return modbus_rx();
}

void setup()
{
  esp_log_level_set("*", ESP_LOG_INFO);
  Serial.begin(115200);
  delay(1000);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!aht.begin(&Wire)) {
    ESP_LOGE(TAG, "Erro ao inicializar o AHTX0!");
  } else {
    ESP_LOGI(TAG, "AHTX0 inicializado com sucesso.");
  }

  modbusSerial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  ESP_LOGI(TAG, "Modbus RS485 iniciado.");

  ESP_LOGI(TAG, "Inicializando LoRa...");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) {
    ESP_LOGE(TAG, "Falha ao iniciar o módulo LoRa!");
    while (true) delay(1000);
  }

  pinMode(ss, OUTPUT);
  uint8_t version = lora_read_reg(0x42);
  ESP_LOGI(TAG, "SX1278 RegVersion: 0x%02X", version);
  if (version != 0x12) {
    ESP_LOGW(TAG, "Valor inesperado em RegVersion. Possível falha na comunicação SPI ou módulo errado.");
  }

  LoRa.setSyncWord(0xA5);
  ESP_LOGI(TAG, "LoRa inicializado com sucesso!");
}

void loop()
{
  unsigned long now = millis();
  if (now - lastSendTime >= sendInterval)
  {
    lastSendTime = now;

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    int16_t irradiance = readPYR20();

    bool temp_ok = !isnan(temp.temperature);
    bool hum_ok  = !isnan(humidity.relative_humidity);
    bool pyr_ok  = irradiance >= 0;

    if (temp_ok && hum_ok && pyr_ok)
    {
      String message = "Temp: " + String(temp.temperature, 2) + " C, " +
                       "Umid: " + String(humidity.relative_humidity, 1) + " %, " +
                       "Irr: " + String(irradiance) + " W/m2";
      ESP_LOGI(TAG, "Enviando: %s", message.c_str());
      LoRa.beginPacket();
      LoRa.print(message);
      LoRa.endPacket();
    }
    else
    {
      if (!temp_ok || !hum_ok)
        ESP_LOGW(TAG, "Falha na leitura do AHT10: T=%.2f, H=%.2f", temp.temperature, humidity.relative_humidity);
      if (!pyr_ok)
        ESP_LOGW(TAG, "Falha na leitura do PYR20 (Modbus)");
    }
  }

  delay(10);
}

