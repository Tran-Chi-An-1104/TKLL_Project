#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "freertos/FreeRTOS.h"
#include <semphr.h>
// ===== Shared sensor values  =====
extern SemaphoreHandle_t GLOB_Mutex;
extern float glob_temperature;
extern float glob_humidity;

// ===== One source of truth for pins =====
#define LED1_PIN 48
#define LED2_PIN 41
#define BOOT_PIN 0

// ===== MQTT topics (giữ chung cho dễ dùng) =====
#define MQTT_CMD_TOPIC     "esp32/lab5/cmd"
#define MQTT_STATUS_TOPIC  "esp32/lab5/status"
#define MQTT_SENSORS_TOPIC "esp32/lab5/sensors"
#define MQTT_WEATHER_TOPIC "esp32/lab5/weather"

#endif
