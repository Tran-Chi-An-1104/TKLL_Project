#ifndef __IOT_CLIENT_H__
#define __IOT_CLIENT_H__

#include "global.h"
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "TaskDHT20.h" 
#include "TaskMainserver.h"

void http_get_weather();
void mqtt_setup();
void mqtt_loop();

// void lab5_task(void *pvParameters);
void TaskIOTClient(void *pvParameters);
#endif
