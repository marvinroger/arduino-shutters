#ifndef Shutters_h
#define Shutters_h

#include <Arduino.h>
#include <EEPROM.h>

namespace ShuttersInternal {
  const int CALIBRATION_LEVELS = 5;
  const int LEVELS = 100;

  enum Stop : unsigned char { STOP_NONE, STOP_NEW_LEVEL, STOP_HALT };
  enum Direction : bool { DIRECTION_DOWN, DIRECTION_UP };
  const unsigned char REQUEST_NONE = 255; // Request must be between 0 and 100, so np
  const unsigned char CALIBRATION_NONE = 255; // CALIBRATION_LEVELS should not be 255, np

  const unsigned char FLAG_KNOWN = 0x80;
  const unsigned char MASK_CURRENT_LEVEL = 0x7F;
}

class Shutters {
private:
  unsigned char _currentLevel;
  bool _moving;
  bool _reached;
  unsigned char _targetLevel;
  unsigned char _requestLevel;
  ShuttersInternal::Stop _stopNeeded;
  unsigned long _timeLastLevel;
  ShuttersInternal::Direction _direction;
  unsigned char _calibration;

  unsigned char _eepromPosition;

  float _delayTotal;
  float _delayOneLevel;
  void (*_upCallback)(void);
  void (*_downCallback)(void);
  void (*_haltCallback)(void);

  void log(const char* text);
  void log(String text);
  void up();
  void down();
  void halt();
  bool savedIsLastLevelKnown();
  void saveLastLevelUnknown();
  unsigned char savedCurrentLevel();
  void saveCurrentLevelAndKnown(unsigned char level);
public:
  Shutters(float delay_total, void (*upCallback)(void), void (*downCallback)(void), void (*haltCallback)(void), unsigned char eeprom_offset = 0);
  bool begin();
  void loop();
  void requestLevel(unsigned char level);
  void stop();
  bool moving();
  bool reached();
  unsigned char currentLevel();
  void eraseSavedState();
};

#endif
