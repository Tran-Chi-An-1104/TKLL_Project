#include "common.h"
#include "TaskDHT20.h"
#include "TaskLCD.h"
#include "TaskLED.h"
#define BUTTON_PIN 7
// put function declarations here:

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.setPins(GPIO_NUM_11, GPIO_NUM_12);
  Wire.begin();

  pinMode(BUTTON_PIN, INPUT);

  DHT20_Mutex = xSemaphoreCreateMutex();
  xTaskCreate(TaskDHT20, "DHT20", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLCD, "LCD", 2048, NULL, 2, NULL);
  xTaskCreate(TaskLED, "LED", 2048, NULL, 2, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}