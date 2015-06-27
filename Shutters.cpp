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

// EEEPROM x0 = 1 if last shutter level known and sure
//         x1: Current shutter level (0 - 100)
//         x2: Request shutter level (0 - 100) -> to backup in case of power outage
byte EEPROM_LAST_LEVEL_KNOWN = 0;
byte EEPROM_CURRENT_LEVEL = EEPROM_LAST_LEVEL_KNOWN + 1;
byte EEPROM_REQUEST_LEVEL = EEPROM_CURRENT_LEVEL + 1;

//const byte FLAG_KNOWN = 0x80;
//const byte MASK_CURRENT_LEVEL = 0x7F;

Shutters::Shutters(byte pin_move, byte pin_direction, float delay_total, bool active_low, byte eeprom_offset) {
  this->moving = false;
  this->request_level = 255;
  this->stop_needed = STOP_NONE;
  this->calibration = -1;

  EEPROM_LAST_LEVEL_KNOWN += eeprom_offset;
  EEPROM_CURRENT_LEVEL += eeprom_offset;
  EEPROM_REQUEST_LEVEL += eeprom_offset;

  this->delay_total = delay_total;
  this->delay_one_level = delay_total / LEVELS;
  this->pin_move = pin_move;
  this->pin_direction = pin_direction;
  this->active = active_low ? LOW : HIGH;
  this->inactive = active_low ? HIGH : LOW;
}

void Shutters::begin() {
  pinMode(this->pin_move, OUTPUT);
  pinMode(this->pin_direction, OUTPUT);
  halt();

  if(!EEPROM.read(EEPROM_LAST_LEVEL_KNOWN)) {
    log("Current level unsure, calibrating...");
    up();
    delay((this->delay_total + this->delay_one_level * CALIBRATION_LEVELS) * 1000);
    halt();
    EEPROM.write(EEPROM_CURRENT_LEVEL, 0);
    EEPROM.write(EEPROM_LAST_LEVEL_KNOWN, 1);
    this->current_level = 0;
    byte request_level_eeprom = EEPROM.read(EEPROM_REQUEST_LEVEL);
    if (request_level_eeprom != 0 && request_level_eeprom <= 100) { // EEPROM might have been populated before
      this->request_level = request_level_eeprom;
    }
  } else {
    this->current_level = EEPROM.read(EEPROM_CURRENT_LEVEL);
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
  EEPROM.write(EEPROM_REQUEST_LEVEL, request_level);
}

void Shutters::stop() {
  if (this->moving) {
    this->stop_needed = STOP_HALT;
  }
}

bool Shutters::isMoving() {
  return this->moving;
}

byte Shutters::getCurrentLevel() {
  return this->current_level;
}

void Shutters::eraseConfig() {
  EEPROM.write(EEPROM_LAST_LEVEL_KNOWN, 0);
  EEPROM.write(EEPROM_REQUEST_LEVEL, 0);
}

void Shutters::loop() {
  // Init request
  if (this->request_level != 255 && this->stop_needed == STOP_NONE) {
    this->target_level = this->request_level;
    this->request_level = 255;

    if (this->target_level != this->current_level) {
      if (this->target_level > this->current_level) {
        down();
      } else {
        up();
      }
      this->time_last_level = millis();
      EEPROM.write(EEPROM_LAST_LEVEL_KNOWN, 0);
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
          EEPROM.write(EEPROM_CURRENT_LEVEL, this->current_level);
          EEPROM.write(EEPROM_LAST_LEVEL_KNOWN, 1);
        }
      } else if (this->stop_needed != STOP_NONE && this->calibration == -1) {
        byte stop_type = stop_needed; // following halt() resets the stop_needed var
        halt();
        if (stop_type == STOP_HALT) {
          log("Stop requested");
          EEPROM.write(EEPROM_CURRENT_LEVEL, this->current_level);
          EEPROM.write(EEPROM_LAST_LEVEL_KNOWN, 1);
        } else if(stop_type == STOP_NEW_LEVEL) {
          log("New target");
        }
      }
    }
  }
}
