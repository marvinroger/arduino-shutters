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

Shutters::Shutters(float delay_total, void (*upCallback)(void), void (*downCallback)(void), void (*haltCallback)(void), byte eeprom_offset) {
  this->moving_ = false;
  this->reached_ = false;
  this->request_level_ = REQUEST_NONE;
  this->stop_needed_ = STOP_NONE;
  this->calibration_ = CALIBRATION_NONE;

  this->eeprom_position_ = EEPROM_POSITION + eeprom_offset;

  this->delay_total_ = delay_total;
  this->delay_one_level_ = delay_total / LEVELS;
  this->upCallback_ = upCallback;
  this->downCallback_ = downCallback;
  this->haltCallback_ = haltCallback;
}

bool Shutters::savedIsLastLevelKnown() {
  byte raw_value = EEPROM.read(this->eeprom_position_);
  if (raw_value & FLAG_KNOWN) {
    return true;
  } else {
    return false;
  }
}

void Shutters::saveLastLevelUnknown() {
  byte current_level = savedCurrentLevel();
  EEPROM.write(this->eeprom_position_, current_level);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

byte Shutters::savedCurrentLevel() {
  byte raw_value = EEPROM.read(this->eeprom_position_);
  byte value = raw_value & MASK_CURRENT_LEVEL;
  return value;
}

void Shutters::saveCurrentLevelAndKnown(byte level) {
  EEPROM.write(this->eeprom_position_, level | FLAG_KNOWN);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

void Shutters::eraseSavedState() {
  EEPROM.write(this->eeprom_position_, 0);
  #ifdef ESP8266
  EEPROM.commit();
  #endif
}

bool Shutters::begin() {
  if(!savedIsLastLevelKnown()) {
    log("Current level unsure, calibrating...");
    up();
    delay((this->delay_total_ + this->delay_one_level_ * CALIBRATION_LEVELS) * 1000);
    halt();
    saveCurrentLevelAndKnown(0);
    this->current_level_ = 0;
    return true;
  } else {
    this->current_level_ = savedCurrentLevel();
    return false;
  }
}

void Shutters::up() {
  this->moving_ = true;
  this->direction_ = DIRECTION_UP;
  this->upCallback_();
  log("Up");
}

void Shutters::down() {
  this->moving_ = true;
  this->direction_ = DIRECTION_DOWN;
  this->downCallback_();
  log("Down");
}

void Shutters::halt() {
  this->stop_needed_ = STOP_NONE;
  this->moving_ = false;
  this->calibration_ = CALIBRATION_NONE;
  this->haltCallback_();
  log("Halt");
}

void Shutters::requestLevel(byte request_level) {
  if (request_level > 100) {
    return;
  }
  if (this->moving_) {
    this->stop_needed_ = STOP_NEW_LEVEL;
  }
  this->request_level_ = request_level;
}

void Shutters::stop() {
  if (this->moving_) {
    this->stop_needed_ = STOP_HALT;
  }
}

bool Shutters::moving() {
  return this->moving_;
}

byte Shutters::currentLevel() {
  return this->current_level_;
}

bool Shutters::reached() {
  bool reached = this->reached_;
  this->reached_ = false;
  return reached;
}

void Shutters::loop() {
  // Init request
  if (this->request_level_ != REQUEST_NONE && this->stop_needed_ == STOP_NONE) {
    this->target_level_ = this->request_level_;
    this->request_level_ = REQUEST_NONE;

    if (this->target_level_ != this->current_level_) {
      saveLastLevelUnknown();

      if (this->target_level_ > this->current_level_) {
        down();
      } else {
        up();
      }
      this->time_last_level_ = millis();
    } else {
      log("Target level already equals current level");
    }
  }

  // Handle request
  if (this->moving_) {
    unsigned long now = millis();

    if (now - this->time_last_level_ >= this->delay_one_level_ * 1000) {
      if (this->calibration_ == CALIBRATION_NONE) {
        if (this->direction_ == DIRECTION_DOWN) {
          this->current_level_ += 1;
        } else {
          this->current_level_ -= 1;
        }
        log(String("Reached level " + String(this->current_level_)));
      }
      this->time_last_level_ = now;

      if (this->current_level_ == this->target_level_) {
        if (this->calibration_ != CALIBRATION_NONE) {
          this->calibration_++;
          log(String("Calibration " + String(this->calibration_) + "/" + String(CALIBRATION_LEVELS)));
        }

        if ((this->current_level_ == 0 || this->current_level_ == 100) && this->calibration_ == CALIBRATION_NONE) {
          this->calibration_ = 0;
          log("Calibrating...");
        } else if (this->calibration_ == CALIBRATION_NONE || this->calibration_ == CALIBRATION_LEVELS) {
          halt();
          log("Reached target");
          saveCurrentLevelAndKnown(this->current_level_);
          this->reached_ = true;
        }
      } else if (this->stop_needed_ != STOP_NONE && this->calibration_ == CALIBRATION_NONE) {
        byte stop_type = this->stop_needed_; // following halt() resets the stop_needed var
        halt();
        if (stop_type == STOP_HALT) {
          log("Stop requested");
          saveCurrentLevelAndKnown(this->current_level_);
          this->reached_ = true;
        } else if(stop_type == STOP_NEW_LEVEL) {
          log("New target");
        }
      }
    }
  }
}
