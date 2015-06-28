#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_OFFSET 0 // Set an offset if you already use a part of the EEPROM

void setup(void)
{
  #ifdef ESP8266
  Serial.begin(115200);
  #else
  Serial.begin(9600);
  #endif

  #ifdef ESP8266
  EEPROM.begin(4); // Only one byte cleared, but 4 is the minimum for begin()
  #endif

  EEPROM.write(EEPROM_OFFSET, 0);

  #ifdef ESP8266
  EEPROM.commit();
  #endif

  Serial.println("Done.")
}

void loop(void)
{

}
