#pragma once

#include "Arduino.h"

namespace ShuttersInternal {
  const uint8_t LEVEL_OFFSET = 28;
  const uint8_t LEVEL_NONE = 255; // level must be between 0 and 100, so np

  class StoredState {
  private:
    // 2^26 but 64 for bitwise (otherwise bitwise shift does not work well)
    uint64_t _upCourseTime;
    // 2^26 but 64 for bitwise
    uint64_t _downCourseTime;
    // 2^7
    uint64_t _level;
    // this leaves 5 bits for another feature for 64 bits

  public:
    StoredState();
    void feed(uint64_t state);
    bool isValid();
    uint8_t getLevel();
    void setLevel(uint8_t level);
    uint32_t getUpCourseTime();
    void setUpCourseTime(uint32_t upCourseTime);
    uint32_t getDownCourseTime();
    void setDownCourseTime(uint32_t downCourseTime);
    uint64_t getState();
    void reset();
  };
}
