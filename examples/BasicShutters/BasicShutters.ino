#include <EEPROM.h>
#include <Shutters.h>

void shuttersUp(void)
{
  Serial.println("Shutters going up.");
  // TODO: Implement the code for the shutters to go up
}

void shuttersDown(void)
{
  Serial.println("Shutters going down.");
  // TODO: Implement the code for the shutters to go down
}

void shuttersHalt(void)
{
  Serial.println("Shutters halted.");
  // TODO: Implement the code for the shutters to halt
}

Shutters shutters(25.1, shuttersUp, shuttersDown, shuttersHalt);

void setup(void)
{
  Serial.begin(9600);
  #ifdef ESP8266
  EEPROM.begin(4); // Only one byte will be used, but 4 is the minimum for ESP8266 begin()
  #endif
  shutters.begin(); // Might take some time first time, as it will open the shutters
  shutters.requestLevel(50); // Go to 50%
}

void loop(void)
{
  shutters.loop();
}
