#include "global.h"
#include "TaskDHT20.h"
#include "TaskLCD.h"
#include "TaskLED.h"
#include "TaskMainserver.h"
#include "TaskIOTClient.h"
#include "TaskLightSleep.h"
#include "TaskTinyML.h"
#include "TaskSerialBridge.h" 
#define BUTTON_PIN 7
// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  enable_I2C();

  pinMode(BUTTON_PIN, INPUT);

  setupAutoLightSleep();

  DHT20_Mutex = xSemaphoreCreateMutex();

  xTaskCreate(TaskDHT20, "DHT20", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLCD, "LCD", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLED, "LED", 2048, NULL, 2, NULL);
  xTaskCreate(TaskMainserver, "Main Server", 8192, NULL, 2, NULL);
  xTaskCreate(TaskIOTClient, "MQTT Task",   8192, NULL, 1, NULL);
  xTaskCreate(TaskTinyML, "TinyML Task",   8192, NULL, 1, NULL);
  xTaskCreate(TaskSerialBridge, "Serial Bridge", 4096, NULL, 1, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}
