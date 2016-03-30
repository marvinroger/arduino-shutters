#include "Shutters.h"

using namespace ShuttersInternal;

void Shutters::log(const char* text) {
  #ifdef DEBUG
  Serial.print("[Shutters] ");
  Serial.println(text);
  #endif
}

void Shutters::log(String text) {
  return log((const char*)text.c_str());
}

Shutters::Shutters(float delay_total, void (*upCallback)(void), void (*downCallback)(void), void (*haltCallback)(void), unsigned char eeprom_offset)
: _moving(false)
, _reached(false)
, _requestLevel(REQUEST_NONE)
, _stopNeeded(STOP_NONE)
, _calibration(CALIBRATION_NONE)
, _eepromPosition(eeprom_offset)
, _delayTotal(delay_total)
, _upCallback(upCallback)
, _downCallback(downCallback)
, _haltCallback(haltCallback)
{
  this->_delayOneLevel = delay_total / LEVELS;
}

bool Shutters::savedIsLastLevelKnown() {
  unsigned char raw_value = EEPROM.read(this->_eepromPosition);
  if (raw_value & FLAG_KNOWN) {
    return true;
  } else {
    return false;
  }
}

void Shutters::saveLastLevelUnknown() {
  unsigned char current_level = savedCurrentLevel();
  EEPROM.write(this->_eepromPosition, current_level);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

unsigned char Shutters::savedCurrentLevel() {
  unsigned char raw_value = EEPROM.read(this->_eepromPosition);
  unsigned char value = raw_value & MASK_CURRENT_LEVEL;
  return value;
}

void Shutters::saveCurrentLevelAndKnown(unsigned char level) {
  EEPROM.write(this->_eepromPosition, level | FLAG_KNOWN);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

void Shutters::eraseSavedState() {
  EEPROM.write(this->_eepromPosition, 0);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

bool Shutters::begin() {
  if(!savedIsLastLevelKnown()) {
    log("Current level unsure, calibrating...");
    up();
    delay((this->_delayTotal + this->_delayOneLevel * CALIBRATION_LEVELS) * 1000);
    halt();
    saveCurrentLevelAndKnown(0);
    this->_currentLevel = 0;
    return true;
  } else {
    this->_currentLevel = savedCurrentLevel();
    return false;
  }
}

void Shutters::up() {
  this->_moving = true;
  this->_direction = DIRECTION_UP;
  this->_upCallback();
  log("Up");
}

void Shutters::down() {
  this->_moving = true;
  this->_direction = DIRECTION_DOWN;
  this->_downCallback();
  log("Down");
}

void Shutters::halt() {
  this->_stopNeeded = STOP_NONE;
  this->_moving = false;
  this->_calibration = CALIBRATION_NONE;
  this->_haltCallback();
  log("Halt");
}

void Shutters::requestLevel(unsigned char level) {
  if (level > 100) {
    return;
  }

  Direction direction = (level > this->_currentLevel) ? DIRECTION_DOWN : DIRECTION_UP;

  if (this->_moving && direction == this->_direction) {
    this->_targetLevel = level;
  } else {
    this->_stopNeeded = STOP_NEW_LEVEL;
    this->_requestLevel = level;
  }
}

void Shutters::stop() {
  if (this->_moving) {
    this->_stopNeeded = STOP_HALT;
  }
}

bool Shutters::moving() {
  return this->_moving;
}

unsigned char Shutters::currentLevel() {
  return this->_currentLevel;
}

bool Shutters::reached() {
  bool reached = this->_reached;
  this->_reached = false;
  return reached;
}

void Shutters::loop() {
  // Init request
  if (this->_requestLevel != REQUEST_NONE && this->_stopNeeded == STOP_NONE) {
    this->_targetLevel = this->_requestLevel;
    this->_requestLevel = REQUEST_NONE;

    if (this->_targetLevel != this->_currentLevel) {
      saveLastLevelUnknown();

      if (this->_targetLevel > this->_currentLevel) {
        down();
      } else {
        up();
      }
      this->_timeLastLevel = millis();
    } else {
      log("Target level already equals current level");
    }
  }

  // Handle request
  if (this->_moving) {
    unsigned long now = millis();

    if (now - this->_timeLastLevel >= this->_delayOneLevel * 1000) {
      if (this->_calibration == CALIBRATION_NONE) {
        if (this->_direction == DIRECTION_DOWN) {
          this->_currentLevel += 1;
        } else {
          this->_currentLevel -= 1;
        }
        log(String("Reached level " + String(this->_currentLevel)));
      }
      this->_timeLastLevel = now;

      if (this->_currentLevel == this->_targetLevel) {
        if (this->_calibration != CALIBRATION_NONE) {
          this->_calibration++;
          log(String("Calibration " + String(this->_calibration) + "/" + String(CALIBRATION_LEVELS)));
        }

        if ((this->_currentLevel == 0 || this->_currentLevel == 100) && this->_calibration == CALIBRATION_NONE) {
          this->_calibration = 0;
          log("Calibrating...");
        } else if (this->_calibration == CALIBRATION_NONE || this->_calibration == CALIBRATION_LEVELS) {
          halt();
          log("Reached target");
          saveCurrentLevelAndKnown(this->_currentLevel);
          this->_reached = true;
        }
      } else if (this->_stopNeeded != STOP_NONE && this->_calibration == CALIBRATION_NONE) {
        unsigned char stop_type = this->_stopNeeded; // following halt() resets the stop_needed var
        halt();
        if (stop_type == STOP_HALT) {
          log("Stop requested");
          saveCurrentLevelAndKnown(this->_currentLevel);
          this->_reached = true;
        } else if(stop_type == STOP_NEW_LEVEL) {
          log("New target");
        }
      }
    }
  }
}
