#include <Arduino.h>

#define ANALOG_PIN 39

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetPinAttenuation(ANALOG_PIN, ADC_11db);

  delay(500);
  (void)analogRead(ANALOG_PIN);  // Bỏ lần đọc đầu tiên
}

void loop() {
  uint16_t raw = analogRead(ANALOG_PIN);
  uint32_t mV = analogReadMilliVolts(ANALOG_PIN);

  Serial.printf(
    "GPIO39 (VN) | RAW = %u | Voltage = %lu mV | %.3f V\n",
    raw,
    (unsigned long)mV,
    mV / 1000.0f
  );

  delay(200);
}

/*
 * Đọc ADC 12-bit và điện áp đã hiệu chuẩn.
 * Dùng ADC_11db cho tín hiệu khoảng 2.45 V.
 * RAW và Voltage được lấy từ hai lần chuyển đổi riêng biệt.
 */