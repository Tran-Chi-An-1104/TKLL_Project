#ifndef LED_H
#define LED_H

#include "common.h"
#include "Adafruit_NeoPixel.h"
#include "TaskDHT20.h"

#define NEO_NUM 1
#define NEO_PIN 45
#define LED_PIN 48

void TaskLED(void *pvParameter);

#endif