#ifndef Shutters_h
#define Shutters_h

#include <Arduino.h>

const int CALIBRATION_LEVELS = 3;
const int LEVELS = 100;

enum Stop : byte { STOP_NONE, STOP_NEW_LEVEL, STOP_HALT };
enum Direction : bool { DIRECTION_DOWN, DIRECTION_UP };
const byte REQUEST_NONE = 255; // Request must be between 0 and 100, so np
const byte CALIBRATION_NONE = 255; // CALIBRATION_LEVELS should not be 255, np

const byte EEPROM_POSITION = 0;
const byte FLAG_KNOWN = 0x80;
const byte MASK_CURRENT_LEVEL = 0x7F;

class Shutters {
private:
  byte current_level_;
  bool moving_;
  byte target_level_;
  byte request_level_;
  Stop stop_needed_;
  unsigned long time_last_level_;
  Direction direction_;
  byte calibration_;

  byte eeprom_position_;

  float delay_total_;
  float delay_one_level_;
  void (*upCallback_)(void);
  void (*downCallback_)(void);
  void (*haltCallback_)(void);

  void log(const char* text);
  void log(String text);
  void up();
  void down();
  void halt();
  bool savedIsLastLevelKnown();
  void saveLastLevelUnknown();
  byte savedCurrentLevel();
  void saveCurrentLevelAndKnown(byte level);
public:
  Shutters(float delay_total, void (*upCallback)(void), void (*downCallback)(void), void (*haltCallback)(void), byte eeprom_offset = 0);
  bool begin();
  void loop();
  void requestLevel(byte level);
  void stop();
  bool moving();
  byte currentLevel();
  void eraseSavedState();
};

#endif
