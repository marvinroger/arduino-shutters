#include <Arduino.h>
#include <EEPROM.h>
#include "Shutters.h"

void Shutters::log(const char* text) {
  #ifdef DEBUG
  Serial.print("[Shutters] ");
  Serial.println(text);
  #endif
}

void Shutters::log(String text) {
  return log((const char*)text.c_str());
}

byte EEPROM_POSITION = 0;

Shutters::Shutters(byte pin_move, byte pin_direction, float delay_total, bool active_low, byte eeprom_offset) {
  this->moving = false;
  this->request_level = 255;
  this->stop_needed = STOP_NONE;
  this->calibration = -1;

  EEPROM_POSITION += eeprom_offset;

  this->delay_total = delay_total;
  this->delay_one_level = delay_total / LEVELS;
  this->pin_move = pin_move;
  this->pin_direction = pin_direction;
  this->active = active_low ? LOW : HIGH;
  this->inactive = active_low ? HIGH : LOW;
}

bool Shutters::savedIsLastLevelKnown() {
  byte raw_value = EEPROM.read(EEPROM_POSITION);
  if (raw_value & FLAG_KNOWN) {
    return true;
  } else {
    return false;
  }
}

void Shutters::saveLastLevelUnknown() {
  byte current_level = savedCurrentLevel();
  EEPROM.write(EEPROM_POSITION, current_level);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

byte Shutters::savedCurrentLevel() {
  byte raw_value = EEPROM.read(EEPROM_POSITION);
  byte value = raw_value & MASK_CURRENT_LEVEL;
  return value;
}

void Shutters::saveCurrentLevelAndKnown(byte level) {
  EEPROM.write(EEPROM_POSITION, level | FLAG_KNOWN);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

void Shutters::eraseSavedState() {
  EEPROM.write(EEPROM_POSITION, 0);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

bool Shutters::begin() {
  pinMode(this->pin_move, OUTPUT);
  pinMode(this->pin_direction, OUTPUT);
  halt();

  #ifdef ESP8266
  EEPROM.begin(4); // 4 bytes minimum, only need 1
  #endif

  if(!savedIsLastLevelKnown()) {
    log("Current level unsure, calibrating...");
    up();
    delay((this->delay_total + this->delay_one_level * CALIBRATION_LEVELS) * 1000);
    halt();
    saveCurrentLevelAndKnown(0);
    this->current_level = 0;
    return true;
  } else {
    this->current_level = savedCurrentLevel();
    return false;
  }
}

void Shutters::up() {
  this->moving = true;
  this->direction = DIRECTION_UP;
  digitalWrite(this->pin_direction, this->active);
  digitalWrite(this->pin_move, this->active);
  log("Up");
}

void Shutters::down() {
  this->moving = true;
  this->direction = DIRECTION_DOWN;
  digitalWrite(this->pin_direction, this->inactive);
  digitalWrite(this->pin_move, this->active);
  log("Down");
}

void Shutters::halt() {
  this->stop_needed = STOP_NONE;
  this->moving = false;
  this->calibration = -1;
  digitalWrite(this->pin_move, this->inactive);
  digitalWrite(this->pin_direction, this->inactive); // To save energy
  log("Halt");
}

void Shutters::requestLevel(byte request_level) {
  if (request_level > 100) {
    return;
  }
  if (this->moving) {
    this->stop_needed = STOP_NEW_LEVEL;
  }
  this->request_level = request_level;
}

void Shutters::stop() {
  if (this->moving) {
    this->stop_needed = STOP_HALT;
  }
}

bool Shutters::areMoving() {
  return this->moving;
}

byte Shutters::currentLevel() {
  return this->current_level;
}

void Shutters::loop() {
  // Init request
  if (this->request_level != 255 && this->stop_needed == STOP_NONE) {
    this->target_level = this->request_level;
    this->request_level = 255;

    if (this->target_level != this->current_level) {
      saveLastLevelUnknown();

      if (this->target_level > this->current_level) {
        down();
      } else {
        up();
      }
      this->time_last_level = millis();
    } else {
      log("Target level already equals current level");
    }
  }

  // Handle request
  if (this->moving) {
    unsigned long now = millis();

    if (now - this->time_last_level >= this->delay_one_level * 1000) {
      if (this->calibration == -1) {
        if (this->direction == DIRECTION_DOWN) {
          this->current_level += 1;
        } else {
          this->current_level -= 1;
        }
        log(String("Reached level " + String(this->current_level)));
      }
      this->time_last_level = now;

      if (this->current_level == this->target_level) {
        if (this->calibration != -1) {
          this->calibration++;
          log(String("Calibration " + String(this->calibration) + "/" + String(CALIBRATION_LEVELS)));
        }

        if ((this->current_level == 0 || this->current_level == 100) && this->calibration == -1) {
          this->calibration = 0;
          log("Calibrating...");
        } else if (this->calibration == -1 || this->calibration == CALIBRATION_LEVELS) {
          halt();
          log("Reached target");
          saveCurrentLevelAndKnown(this->current_level);
        }
      } else if (this->stop_needed != STOP_NONE && this->calibration == -1) {
        byte stop_type = stop_needed; // following halt() resets the stop_needed var
        halt();
        if (stop_type == STOP_HALT) {
          log("Stop requested");
          saveCurrentLevelAndKnown(this->current_level);
        } else if(stop_type == STOP_NEW_LEVEL) {
          log("New target");
        }
      }
    }
  }
}
