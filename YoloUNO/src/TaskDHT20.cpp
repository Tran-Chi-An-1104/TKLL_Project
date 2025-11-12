//Task to communicate and read from DHT20 sensor   

#include "TaskDHT20.h"
#include "global.h"
static QueueHandle_t tQueue = NULL;
SemaphoreHandle_t DHT20_Mutex;
DHT20_data dht20;

void TaskDHT20(void *pvParameter){
    DHT20 DHT20;
    memset(&dht20, 0, sizeof(dht20));

    while(!DHT20.begin()){
        Serial.println("Trying to connect");
        vTaskDelay(1000);
    }

    while (true){
        DHT20.read();
        float temp = DHT20.getTemperature();
        float humi = DHT20.getHumidity();

        //Take mutex prior update struct, wait for 0 ticks, if it can't obtain mutex, don't update
        if(xSemaphoreTake(DHT20_Mutex, 0)){
            dht20.temp = temp;
            dht20.humidity = humi;
            glob_temperature = temp;
            glob_humidity    = humi;
            xSemaphoreGive(DHT20_Mutex);
        }
        if (GLOB_Mutex && xSemaphoreTake(GLOB_Mutex, pdMS_TO_TICKS(5))) {
            glob_temperature = temp;
            glob_humidity    = humi;
            xSemaphoreGive(GLOB_Mutex);
        }
        vTaskDelay(1000);
    }
}