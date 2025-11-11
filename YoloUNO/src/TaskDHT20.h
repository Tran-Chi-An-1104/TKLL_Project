#ifndef DHT20_H
#define DHT20_H

#include "common.h"
#include "Wire.h"
#include "DHT20.h"

typedef struct DHT20_struct{
  float temp;
  float humidity;
}DHT20_data;

extern DHT20_data dht20;

void TaskDHT20(void *pvParameter);

#endif