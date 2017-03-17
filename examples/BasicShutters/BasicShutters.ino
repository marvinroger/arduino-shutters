#include <Shutters.h>

const unsigned long courseTime = 30 * 1000;
const float calibrationRatio = 0.1;

void shuttersUp() {
  Serial.println("Shutters going up.");
  // TODO: Implement the code for the shutters to go up
}

void shuttersDown() {
  Serial.println("Shutters going down.");
  // TODO: Implement the code for the shutters to go down
}

void shuttersHalt() {
  Serial.println("Shutters halted.");
  // TODO: Implement the code for the shutters to halt
}

uint8_t shuttersGetState() {
  return 255;
}

void shuttersSetState(uint8_t state) {
  Serial.print("Saving state ");
  Serial.print(state);
  Serial.println(".");
}

void onShuttersLevelReached(uint8_t level) {
  Serial.print("Shutters at ");
  Serial.print(level);
  Serial.println("%");
}

Shutters shutters(courseTime, shuttersUp, shuttersDown, shuttersHalt, shuttersGetState, shuttersSetState, calibrationRatio, onShuttersLevelReached);

void setup() {
  Serial.begin(9600);
  shutters.begin();

  shutters.setLevel(50); // Go to 50%
}

void loop() {
  shutters.loop();
}
