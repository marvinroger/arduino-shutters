#include <Shutters.h>
#include <EEPROM.h>

const byte eepromOffset = 0;
const unsigned long upCourseTime = 30 * 1000;
const unsigned long downCourseTime = 45 * 1000;
const float calibrationRatio = 0.1;

void shuttersOperationHandler(Shutters* s, ShuttersOperation operation) {
  switch (operation) {
    case ShuttersOperation::UP:
      Serial.println("Shutters going up.");
      // TODO: Implement the code for the shutters to go up
      break;
    case ShuttersOperation::DOWN:
      Serial.println("Shutters going down.");
      // TODO: Implement the code for the shutters to go down
      break;
    case ShuttersOperation::HALT:
      Serial.println("Shutters halting.");
      // TODO: Implement the code for the shutters to halt
      break;
  }
}

char* shuttersReadStateHandler(Shutters* shutters, byte length) {
  char state[length + 1];
  for (uint8_t i = 0; i < length; i++) {
    state[i] = EEPROM.read(eepromOffset + i);
  }
  state[length] = '\0';

  return strdup(state);
}

void shuttersWriteStateHandler(Shutters* shutters, const char* state, byte length) {
  for (byte i = 0; i < length; i++) {
    EEPROM.write(eepromOffset + i, state[i]);
    #ifdef ESP8266
    EEPROM.commit();
    #endif
  }
}

void onShuttersLevelReached(Shutters* shutters, byte level) {
  Serial.print("Shutters at ");
  Serial.print(level);
  Serial.println("%");
}

Shutters shutters;

void setup() {
  Serial.begin(9600);
  delay(100);
  #ifdef ESP8266
  EEPROM.begin(512);
  #endif
  Serial.println();
  Serial.println("*** Starting ***");
  shutters
    .setOperationHandler(shuttersOperationHandler)
    .setReadStateHandler(shuttersReadStateHandler)
    .setWriteStateHandler(shuttersWriteStateHandler)
    .setCourseTime(upCourseTime, downCourseTime)
    .onLevelReached(onShuttersLevelReached)
    .begin()
    .setLevel(30); // Go to 30%
}

void loop() {
  shutters.loop();

  if (Serial.available() > 0) {
    int level = Serial.parseInt();

    Serial.print("Going to level ");
    Serial.println(level);
    shutters.setLevel(level);
  }
}
