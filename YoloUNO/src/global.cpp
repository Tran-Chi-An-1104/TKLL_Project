#include <Arduino.h>
#include "global.h"

SemaphoreHandle_t GLOB_Mutex = nullptr;

float glob_temperature = 0.0f;
float glob_humidity    = 0.0f;
