#ifndef Shutters_h
#define Shutters_h

const int CALIBRATION_LEVELS = 3;
const int LEVELS = 100;

enum Stop : byte { STOP_NONE, STOP_NEW_LEVEL, STOP_HALT };

enum Direction : bool { DIRECTION_DOWN, DIRECTION_UP };

const byte FLAG_KNOWN = 0x80;
const byte MASK_CURRENT_LEVEL = 0x7F;

class Shutters {
private:
  byte current_level;
  bool moving;
  byte target_level;
  byte request_level;
  byte stop_needed;
  unsigned long time_last_level;
  bool direction;
  int calibration;

  byte pin_move;
  byte pin_direction;
  float delay_total;
  float delay_one_level;
  uint32_t active;
  uint32_t inactive;

  void log(const char*);
  void log(String);
  void up();
  void down();
  void halt();
  bool savedIsLastLevelKnown();
  void saveLastLevelUnknown();
  byte savedCurrentLevel();
  void saveCurrentLevelAndKnown(byte);
public:
  Shutters(byte, byte, float, bool = false, byte = 0);
  bool begin();
  void loop();
  void requestLevel(byte);
  void stop();
  bool areMoving();
  byte currentLevel();
  void eraseSavedState();
};

#endif
