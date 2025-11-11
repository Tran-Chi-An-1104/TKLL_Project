#include "TaskLED.h"

void TaskLED(void *pvParameter){
    pinMode(LED_PIN, OUTPUT);
    pinMode(NEO_PIN, OUTPUT);

    Adafruit_NeoPixel LED_Strip(NEO_NUM, NEO_PIN, NEO_GRB + NEO_KHZ800);
    LED_Strip.begin();
    LED_Strip.clear();
    LED_Strip.show();



    while(true){
        LED_Strip.setPixelColor(0, LED_Strip.Color(0, 0, 255));
        LED_Strip.show();

        vTaskDelay(500);
        LED_Strip.setPixelColor(0, LED_Strip.Color(0, 0, 0));
        LED_Strip.show();

        vTaskDelay(500);
    }
}