#ifndef __IOT_CLIENT_H__
#define __IOT_CLIENT_H__

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

void http_get_weather();
void mqtt_setup();
void mqtt_loop();

void lab5_task(void *pvParameters);
void mqtt_task(void *);
#endif
