#include "StoredState.hpp"

using namespace ShuttersInternal;

StoredState::StoredState()
: _upCourseTime(0)
, _downCourseTime(0)
, _level(0)
{
}

void StoredState::feed(const char* state) {
  uint64_t stateNumber = 0;

  for (int i = 0; i < STATE_LENGTH; i++) {
    char c = state[i];
    if (c < '0' || c > '9') break;

    stateNumber *= 10;
    stateNumber += (c - '0');
  }

  const uint64_t upCourseTime = stateNumber >> 38;
  const uint64_t downCourseTime = (stateNumber << 26) >> 38;
  const uint64_t rawLevel = (stateNumber << 52) >> 57;

  _upCourseTime = upCourseTime;
  _downCourseTime = downCourseTime;

  if (rawLevel < LEVEL_OFFSET) {
    _level = LEVEL_NONE;
  } else {
    _level = rawLevel - LEVEL_OFFSET;
  }
}

bool StoredState::isValid() {
  bool upCourseTimeValid = _upCourseTime > 0;
  bool downCourseTimeValid = _downCourseTime > 0;
  bool levelValid = _level >= 0 && _level <= 100;

  return upCourseTimeValid && downCourseTimeValid && levelValid;
}

uint8_t StoredState::getLevel() {
  return _level;
}

void StoredState::setLevel(uint8_t level) {
  _level = level;
}

uint32_t StoredState::getUpCourseTime() {
  return _upCourseTime;
}

void StoredState::setUpCourseTime(uint32_t upCourseTime) {
  if (upCourseTime > 67108864UL || upCourseTime == 0) return; // max value for 26 bits

  _upCourseTime = upCourseTime;
}

uint32_t StoredState::getDownCourseTime() {
  return _downCourseTime;
}

void StoredState::setDownCourseTime(uint32_t downCourseTime) {
  if (downCourseTime > 67108864UL || downCourseTime == 0) return; // max value for 26 bits

  _downCourseTime = downCourseTime;
}

const char* StoredState::getState() {
  uint64_t upCourseTime = _upCourseTime << 38;
  uint64_t downCourseTime = _downCourseTime << 12;
  uint64_t level = (_level + LEVEL_OFFSET) << 5;

  uint64_t stateNumber = upCourseTime | downCourseTime | level;

  uint8_t digit;
  for (int i = 0; i < STATE_LENGTH; i++) {
    digit = stateNumber % 10;
    stateNumber /= 10;
    _state[(STATE_LENGTH - 1) - i] = '0' + digit;
  }

  _state[STATE_LENGTH] = '\0';

  return _state;
}

void StoredState::reset() {
  _upCourseTime = 0;
  _downCourseTime = 0;
  _level = 0;
}
