#include <Arduino.h>
#include <EEPROM.h>
#include <Shutters.h>

Shutters shutters(2, 3, 25.1); // move and direction relays on pin 2 and 3, 25.1 secs a full course

void setup(void)
{
  shutters.begin(); // Might take some time first time, as it will open the shutters
  shutters.requestLevel(50); // Go to 50%
}

void loop(void)
{
  shutters.loop();
}
