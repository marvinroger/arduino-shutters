#include <Shutters.h>

const unsigned long courseTime = 30 * 1000;
const float calibrationRatio = 0.1;

void shuttersUp(Shutters* shutters) {
  Serial.println("Shutters going up.");
  // TODO: Implement the code for the shutters to go up
}

void shuttersDown(Shutters* shutters) {
  Serial.println("Shutters going down.");
  // TODO: Implement the code for the shutters to go down
}

void shuttersHalt(Shutters* shutters) {
  Serial.println("Shutters halted.");
  // TODO: Implement the code for the shutters to halt
}

uint32_t shuttersGetState(Shutters* shutters) {
  return 255;
}

void shuttersSetState(Shutters* shutters, uint32_t state) {
  Serial.print("Saving state ");
  Serial.print(state);
  Serial.println(".");
}

void onShuttersLevelReached(Shutters* shutters, uint8_t level) {
  Serial.print("Shutters at ");
  Serial.print(level);
  Serial.println("%");
}

Shutters shutters(shuttersUp, shuttersDown, shuttersHalt, shuttersGetState, shuttersSetState);

void setup() {
  Serial.begin(9600);
  shutters.begin();

  shutters.setLevel(50); // Go to 50%
}

void loop() {
  shutters.loop();
}
