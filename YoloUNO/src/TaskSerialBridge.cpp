#include "TaskSerialBridge.h"
#include "global.h" 

bool isSending = true;

void TaskSerialBridge(void *pvParameter) {
    Serial.println("[SerialBridge] Task Started. Waiting for commands...");

    DHT20_data local_dht;

    while (true) {
        // --- 1. Wait command from RPi (Serial) ---
        if (Serial.available() > 0) {
            String command = Serial.readStringUntil('\n');
            command.trim();

            if (command == "PAUSE") {
                isSending = false;
                Serial.println("CMD: PAUSED");
            } 
            else if (command == "RESUME") {
                isSending = true;
                Serial.println("CMD: RESUMED");
            }
            else if (command.startsWith("SLEEP:")) {
                String timeStr = command.substring(6);
                int sleepSeconds = timeStr.toInt();
                
                if (sleepSeconds > 0) {
                    Serial.printf("CMD: Sleeping for %d seconds...\n", sleepSeconds);
                    Serial.flush(); 
                    vTaskDelay(pdMS_TO_TICKS(sleepSeconds * 1000));
                    
                    Serial.println("CMD: Woke up!");
                }
            }
        }

        // --- 2. Send data ---
        if (isSending) {
            if (xSemaphoreTake(DHT20_Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                local_dht = dht20; 
                xSemaphoreGive(DHT20_Mutex);

                Serial.print("temp:");
                Serial.println(local_dht.temp, 1);

                vTaskDelay(pdMS_TO_TICKS(50)); 

                Serial.print("hum:");
                Serial.println(local_dht.humidity, 1);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}