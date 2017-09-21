#include "StoredState.hpp"

using namespace ShuttersInternal;

StoredState::StoredState()
: _upCourseTime(0)
, _downCourseTime(0)
, _level(0)
{
}

void StoredState::feed(uint64_t state) {
  const uint64_t upCourseTime = state >> 38;
  const uint64_t downCourseTime = (state << 26) >> 38;
  const uint64_t rawLevel = (state << 52) >> 57;

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

uint64_t StoredState::getState() {
  uint64_t upCourseTime = _upCourseTime << 38;
  uint32_t downCourseTime = _downCourseTime << 12;
  uint32_t level = (_level + LEVEL_OFFSET) << 7;

  return upCourseTime | downCourseTime | level;
}

void StoredState::reset() {
  _upCourseTime = 0;
  _downCourseTime = 0;
  _level = 0;
}
