#include "TaskIOTClient.h"
#include "global.h"

// =========================
// CORE IOT / THINGSBOARD MQTT BASIC
// =========================
static const char* COREIOT_HOST   = "app.coreiot.io";
static const int   COREIOT_PORT   = 1883;         
static const char* MQTT_CLIENT_ID = "hackerbu";
static const char* MQTT_USERNAME  = "hackerbu";
static const char* MQTT_PASSWORD  = "hackerbu";

static WiFiClient   netClient;
static PubSubClient mqtt(netClient);

static uint32_t lastReconnectTry    = 0;
static uint32_t reconnectIntervalMs = 2000;

static double DEF_LAT = 10.8231;
static double DEF_LON = 106.6297;

static const char* TB_TELEMETRY_TOPIC   = "v1/devices/me/telemetry";
static const char* TB_RPC_REQUEST_TOPIC = "v1/devices/me/rpc/request/+";

// -------------------------------------------------------------
// HELPER: Publish status {"status":"..."}
// -------------------------------------------------------------
static void mqtt_publish_status(const char* msg){
    char buf[96];
    snprintf(buf, sizeof(buf), "{\"status\":\"%s\"}", msg);  \
    mqtt.publish(TB_TELEMETRY_TOPIC, buf, false);
}

// -------------------------------------------------------------
// WEATHER HTTP GET (Open-Meteo) + publish telemetry
// -------------------------------------------------------------
static bool fetch_and_publish_weather(double lat, double lon){
    if (WiFi.status() != WL_CONNECTED) return false;

    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 6) +
                 "&longitude=" + String(lon, 6) + "&current_weather=true";

    WiFiClientSecure https;
    https.setInsecure();
    HTTPClient http;

    if (!http.begin(https, url)) {
        mqtt_publish_status("weather_http_begin_fail");
        return false;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        char buf[64];
        snprintf(buf, sizeof(buf), "weather_http_%d", code);
        mqtt_publish_status(buf);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        mqtt_publish_status("weather_json_parse_error");
        return false;
    }

    JsonObject cur = doc["current_weather"];
    if (cur.isNull()) {
        mqtt_publish_status("weather_no_current_weather");
        return false;
    }

    float temperature = cur["temperature"]   | NAN;
    float windspeed   = cur["windspeed"]     | NAN;
    float winddir     = cur["winddirection"] | NAN;
    const char* time  = cur["time"]          | "";

    char out[192];
    snprintf(out, sizeof(out),
             "{\"weather\":true,\"lat\":%.5f,\"lon\":%.5f,\"temp\":%.1f,"
             "\"wind_kmh\":%.1f,\"wind_deg\":%.0f,\"time\":\"%s\"}",
             lat, lon,
             (double)temperature,
             (double)windspeed,
             (double)winddir,
             time);

    mqtt.publish(TB_TELEMETRY_TOPIC, out, false);
    return true;
}

// -------------------------------------------------------------
// BRIDGE COMMAND HANDLER 
// -------------------------------------------------------------
extern bool led1_state;
extern bool led2_state;

static void handle_cmd(String payload){
    payload.toLowerCase();
    payload.trim();   

    if (payload == "hello") {
        mqtt_publish_status("hello_from_esp32");
    }
    else if (payload.startsWith("weather")) {
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
        mqtt_publish_status(ok ? "weather_ok" : "weather_error");
    }

    else if (payload == "toggle led1") {
        led1_state = !led1_state;
        digitalWrite(LED1_PIN, led1_state);
        mqtt_publish_status(led1_state ? "led1_on" : "led1_off");
    }

    else if (payload == "toggle led2") {
        led2_state = !led2_state;
        digitalWrite(LED2_PIN, led2_state);
        mqtt_publish_status(led2_state ? "led2_on" : "led2_off");
    }
    else {
        mqtt_publish_status("unknown_cmd");
    }
}

// -------------------------------------------------------------
// MQTT CALLBACK (RPC + fallback command)
// -------------------------------------------------------------
static void on_mqtt_message(char* topic, byte* payload, unsigned int len) {
    String s;
    s.reserve(len);
    for (unsigned int i = 0; i < len; i++) s += (char)payload[i];

    Serial.printf("[MQTT] %s -> %s\n", topic, s.c_str());

    String topicStr(topic);

    if (topicStr.startsWith("v1/devices/me/rpc/request/")) {

        int lastSlash = topicStr.lastIndexOf('/');
        String reqId = topicStr.substring(lastSlash + 1);
        String respTopic = "v1/devices/me/rpc/response/" + reqId;

        JsonDocument doc;
        if (!deserializeJson(doc, s)) {
            String method = doc["method"] | "";

            if (method == "sendCommand") {
                String cmd = doc["params"]["command"] | "";
                cmd.trim();   

                Serial.println("[SHELL CMD] => " + cmd);

                handle_cmd(cmd);  

                String resp = "{\"output\":\"hello_from_esp32\"}";  
                mqtt.publish(respTopic.c_str(), resp.c_str(), false);  
                return;
            }

            if (method.length() > 0) {
                handle_cmd(method);
            }

            mqtt.publish(respTopic.c_str(), "{\"status\":\"ok\"}", false);
        }
        else {
            mqtt.publish(respTopic.c_str(), "{\"status\":\"json_error\"}", false);
        }
        return;
    }

    handle_cmd(s);
}


// -------------------------------------------------------------
// MQTT CONNECT (MQTT BASIC: clientID / username / password)
// -------------------------------------------------------------
static bool mqtt_connect_once(){
    if (WiFi.status() != WL_CONNECTED) return false;

    mqtt.setServer(COREIOT_HOST, COREIOT_PORT);
    mqtt.setCallback(on_mqtt_message);
    mqtt.setBufferSize(1024);

    Serial.printf("Connecting MQTT BASIC to %s:%d ...\n",
                  COREIOT_HOST, COREIOT_PORT);

    bool ok = mqtt.connect(
        MQTT_CLIENT_ID,   
        MQTT_USERNAME,    
        MQTT_PASSWORD    
    );

    if (ok){
        Serial.println("MQTT connected (BASIC auth).");

        mqtt.subscribe(TB_RPC_REQUEST_TOPIC, 1);

        mqtt_publish_status("online");
    } else {
        Serial.printf("MQTT failed (state=%d)\n", mqtt.state());
    }

    return ok;
}

// -------------------------------------------------------------
// PUBLIC API
// -------------------------------------------------------------
void mqtt_setup(){
    lastReconnectTry  = millis();
}

void mqtt_loop(){
    if (!mqtt.connected()) {
        uint32_t now = millis();
        if (now - lastReconnectTry >= reconnectIntervalMs) {
            lastReconnectTry = now;

            if (mqtt_connect_once()) {
                reconnectIntervalMs = 2000;
            } else {
                if (reconnectIntervalMs < 10000)
                    reconnectIntervalMs += 1000;
            }
        }
    }
    else {
        mqtt.loop();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
}

void TaskIOTClient(void *){
    mqtt_setup();
    for(;;){
        mqtt_loop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
