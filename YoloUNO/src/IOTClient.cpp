#include "IOTClient.h"
#include "global.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ====== HIVE MQ CREDENTIALS ======
static const char* HIVEMQ_HOST = "556cf1d3be9443d8be5699268d114bb9.s1.eu.hivemq.cloud";
static const int   HIVEMQ_PORT = 8883;
static const char* HIVEMQ_USER = "hivemq.webclient.1762834959920";
static const char* HIVEMQ_PASS = "9mqOh#1,0<AlaeL&RXI2";

// TLS client + MQTT
static WiFiClientSecure tlsClient;
static PubSubClient     mqtt(tlsClient);

// Backoff reconnect
static uint32_t lastReconnectTry = 0;
static uint32_t reconnectIntervalMs = 2000;

// Default location for weather
static double DEF_LAT = 10.8231;   // Ho Chi Minh City
static double DEF_LON = 106.6297;

// ---- helpers ----
static void mqtt_publish_status(const char* msg){
  mqtt.publish(MQTT_STATUS_TOPIC, msg, true);
}

static void mqtt_publish_sensors(){
  float t, h;
  if (GLOB_Mutex && xSemaphoreTake(GLOB_Mutex, pdMS_TO_TICKS(5))) {
    t = glob_temperature;
    h = glob_humidity;
    xSemaphoreGive(GLOB_Mutex);
  } else {
    t = glob_temperature; 
    h = glob_humidity;
  }

  char buf[96];
  snprintf(buf, sizeof(buf), "{\"temp\":%.1f,\"hum\":%.0f}", (double)t, (double)h);
  mqtt.publish(MQTT_SENSORS_TOPIC, buf, false);
}

static bool fetch_and_publish_weather(double lat, double lon){
  if (WiFi.status() != WL_CONNECTED) return false;

  // Open-Meteo current weather (free, no API key)
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
               "&longitude=" + String(lon, 6) + "&current_weather=true";

  WiFiClientSecure https;
  https.setInsecure(); // DEV: bỏ verify cert cho nhanh
  HTTPClient http;
  if (!http.begin(https, url)) {
    mqtt.publish(MQTT_WEATHER_TOPIC, "{\"error\":\"http_begin_fail\"}", false);
    return false;
  }

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"error\":\"http_%d\"}", code);
    mqtt.publish(MQTT_WEATHER_TOPIC, buf, false);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;     
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    mqtt.publish(MQTT_WEATHER_TOPIC, "{\"error\":\"json_parse\"}", false);
    return false;
  }

  JsonObject cur = doc["current_weather"];
  if (cur.isNull()){
    mqtt.publish(MQTT_WEATHER_TOPIC, "{\"error\":\"no_current_weather\"}", false);
    return false;
  }

  float temperature = cur["temperature"]   | NAN; // °C
  float windspeed   = cur["windspeed"]     | NAN; // km/h
  float winddir     = cur["winddirection"] | NAN; // deg
  const char* time  = cur["time"]          | "";

  char out[192];
  snprintf(out, sizeof(out),
    "{\"lat\":%.5f,\"lon\":%.5f,\"temp\":%.1f,\"wind_kmh\":%.1f,"
    "\"wind_deg\":%.0f,\"time\":\"%s\"}",
    lat, lon, (double)temperature, (double)windspeed, (double)winddir, time);
  mqtt.publish(MQTT_WEATHER_TOPIC, out, false);

  return true;
}

static void handle_cmd(String payload){
  payload.toLowerCase();

  if      (payload == "led1_on")  { digitalWrite(LED1_PIN, HIGH); mqtt_publish_status("LED1=ON"); }
  else if (payload == "led1_off") { digitalWrite(LED1_PIN, LOW);  mqtt_publish_status("LED1=OFF"); }
  else if (payload == "hello")    { mqtt_publish_status("hello from ESP32"); }
  else if (payload == "sensors")  { mqtt_publish_sensors(); }
  else if (payload.startsWith("weather")){
    // "weather"  hoặc  "weather <lat>,<lon>"
    double lat = DEF_LAT, lon = DEF_LON;
    int sp = payload.indexOf(' ');
    if (sp > 0){
      String coords = payload.substring(sp + 1);
      int comma = coords.indexOf(',');
      if (comma > 0){
        lat = coords.substring(0, comma).toFloat();
        lon = coords.substring(comma + 1).toFloat();
      }
    }
    bool ok = fetch_and_publish_weather(lat, lon);
    if (!ok) mqtt_publish_status("weather_error");
    else     mqtt_publish_status("weather_ok");
  }
  else {
    mqtt_publish_status("unknown_cmd");
  }
}

static void on_mqtt_message(char* topic, byte* payload, unsigned int len){
  String s; s.reserve(len);
  for (unsigned int i=0;i<len;i++) s += (char)payload[i];
  Serial.printf("[MQTT] %s -> %s\n", topic, s.c_str());
  handle_cmd(s);
}

static bool mqtt_connect_once(){
  if (WiFi.status() != WL_CONNECTED) return false;

  tlsClient.setInsecure(); // DEV ONLY

  mqtt.setServer(HIVEMQ_HOST, HIVEMQ_PORT);
  mqtt.setCallback(on_mqtt_message);
  mqtt.setBufferSize(1024);

  String clientId = "esp32s3-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.printf("Connecting MQTT to %s:%d ...\n", HIVEMQ_HOST, HIVEMQ_PORT);

  // PubSubClient connect with username/password + last will
  bool ok = mqtt.connect(
      clientId.c_str(),
      HIVEMQ_USER,
      HIVEMQ_PASS,
      MQTT_STATUS_TOPIC, // will topic
      1,                 // will QoS
      true,              // will retain
      "offline"          // will message
  );

  if (ok){
    mqtt.publish(MQTT_STATUS_TOPIC, "online", true);
    mqtt.subscribe(MQTT_CMD_TOPIC, 1);
    Serial.println("MQTT connected.");
  } else {
    Serial.printf("MQTT failed (state=%d)\n", mqtt.state());
  }
  return ok;
}

// ---- public API ----
void mqtt_setup(){
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  lastReconnectTry = millis();
}

void mqtt_loop(){
  if (!mqtt.connected()){
    uint32_t now = millis();
    if (now - lastReconnectTry >= reconnectIntervalMs){
      lastReconnectTry = now;
      if (mqtt_connect_once()){
        reconnectIntervalMs = 2000;
      } else {
        if (reconnectIntervalMs < 10000) reconnectIntervalMs += 1000;
      }
    }
  } else {
    mqtt.loop();
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}
void mqtt_task(void *){
  mqtt_setup();
  for(;;){
    mqtt_loop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}