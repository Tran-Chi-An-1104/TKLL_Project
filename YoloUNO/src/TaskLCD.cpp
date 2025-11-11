//Task to PRINT

#include "TaskLCD.h"
#include "TaskDHT20.h"

void TaskLCD(void *pvParameter){
  LiquidCrystal_I2C lcd(33,16,2);
  lcd.init();
  lcd.backlight();

  DHT20_data dht20_LOCAL;
  memset(&dht20_LOCAL, 0, sizeof(dht20_LOCAL));
  while (true)
  {
    //Wait for 5 ticks for mutex to be released to update local value
    if(xSemaphoreTake(DHT20_Mutex, 5)){
      dht20_LOCAL = dht20;
      xSemaphoreGive(DHT20_Mutex);
    }

    String print_buffer;
    print_buffer = "Temp: " + String(dht20_LOCAL.temp) + "\n" + "Humidity: " + String(dht20_LOCAL.humidity) + "\n";
    Serial.print(print_buffer);
    Serial.println();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20_LOCAL.temp);
    lcd.setCursor(0, 1);
    lcd.print(dht20_LOCAL.humidity);

    vTaskDelay(1500);
  }
}