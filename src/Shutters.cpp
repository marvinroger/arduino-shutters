#include "Shutters.hpp"

// #define DEBUG
#ifdef DEBUG
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
#else
  #define DPRINT(...)
  #define DPRINTLN(...)
#endif

using namespace ShuttersInternal;

Shutters::Shutters(OperationFunction up, OperationFunction down, OperationFunction halt, GetStateFunction getState, SetStateFunction setState)
: _upCourseTime(0)
, _downCourseTime(0)
, _calibrationRatio(0.1)
, _state(STATE_IDLE)
, _stateTime(0)
, _direction(DIRECTION_UP)
, _storedState()
, _currentLevel(LEVEL_NONE)
, _targetLevel(LEVEL_NONE)
, _safetyDelay(false)
, _safetyDelayTime(0)
, _init(false)
, _reset(true)
, _upFunction(up)
, _downFunction(down)
, _haltFunction(halt)
, _getStateFunction(getState)
, _setStateFunction(setState)
, _levelReachedCallback(nullptr)
{
}

void Shutters::_up() {
  DPRINTLN(F("Shutters: going up"));
  _upFunction(this);
}

void Shutters::_down() {
  DPRINTLN(F("Shutters: going down"));
  _downFunction(this);
}

void Shutters::_halt() {
  DPRINTLN(F("Shutters: halting"));
  _haltFunction(this);
  _setSafetyDelay();
}

void Shutters::_setSafetyDelay() {
  _safetyDelayTime = millis();
  _safetyDelay = true;
}

void Shutters::_notifyLevel() {
  DPRINT(F("Shutters: notifying level "));
  DPRINTLN(_currentLevel);
  if (_levelReachedCallback) _levelReachedCallback(this, _currentLevel);
}

uint32_t Shutters::getUpCourseTime() {
  return _upCourseTime;
}

uint32_t Shutters::getDownCourseTime() {
  return _downCourseTime;
}


void Shutters::setCourseTime(uint32_t upCourseTime, uint32_t downCourseTime) {
  if (!_reset) {
    return;
  }

  if (!_init) {
    _storedState.feed(_getStateFunction(this));
    if (_storedState.isValid()) {
      DPRINTLN(F("Shutters: Stored state is valid"));
      _currentLevel = _storedState.getLevel();
      _notifyLevel();
    } else {
      DPRINTLN(F("Stored state is invalid"));
    }

    _init = true;
  }

  if (upCourseTime > 67108864UL|| upCourseTime == 0) return; // max value for 26 bits
  // if down course time is not set, consider it's the same as up
  if (downCourseTime == 0) downCourseTime = upCourseTime;
  if (downCourseTime > 67108864UL) return; // max value for 26 bits

  if (upCourseTime != _storedState.getUpCourseTime() || downCourseTime != _storedState.getDownCourseTime()) {
    DPRINTLN(F("Shutters: course time is not the same, invalidating stored state"));
    _storedState.setLevel(LEVEL_NONE);
    _currentLevel = LEVEL_NONE;
  }

  _upCourseTime = upCourseTime;
  _upStepTime = upCourseTime / LEVELS;
  _upCalibrationTime = upCourseTime * _calibrationRatio;
  _storedState.setUpCourseTime(upCourseTime);

  _downCourseTime = downCourseTime;
  _downStepTime = downCourseTime / LEVELS;
  _downCalibrationTime = downCourseTime * _calibrationRatio;
  _storedState.setDownCourseTime(downCourseTime);

  _setStateFunction(this, _storedState.getState());
}

float Shutters::getCalibrationRatio() {
  return _calibrationRatio;
}

void Shutters::setCalibrationRatio(float calibrationRatio) {
  _calibrationRatio = calibrationRatio;
  _upCalibrationTime = _upCourseTime * calibrationRatio;
  _downCalibrationTime = _downCourseTime * calibrationRatio;
}

void Shutters::onLevelReached(ShuttersInternal::LevelReachedCallback callback) {
  _levelReachedCallback = callback;
}

void Shutters::begin() {
  if (_upCourseTime == 0 || _downCourseTime == 0) return;

  _reset = false;
}

void Shutters::setLevel(uint8_t level) {
  if (_reset) {
    return;
  }

  if (level > 100) {
    return;
  }

  if (_state == STATE_IDLE && level == _currentLevel) return;
  if ((_state == STATE_TARGETING || _state == STATE_NORMALIZING) && level == _targetLevel) return; // normalizing check useless, but avoid following lines overhead

  _targetLevel = level;
  Direction direction = (_targetLevel > _currentLevel) ? DIRECTION_DOWN : DIRECTION_UP;
  if (_state == STATE_TARGETING && _direction != direction) {
    _state = STATE_NORMALIZING;
  }
}

void Shutters::stop() {
  if (_reset) return;

  if (_state == STATE_IDLE) return;

  _targetLevel = LEVEL_NONE;
  if (_state == STATE_TARGETING) {
    _state = STATE_NORMALIZING;
  }
}

void Shutters::loop() {
  if (_reset) return;

  if (_safetyDelay) {
    if (millis() - _safetyDelayTime >= SAFETY_DELAY) {
      DPRINTLN(F("Shutters: end of safety delay"));
      _safetyDelay = false;
    }

    return;
  }

  // here, we're safe for relays

  if (_currentLevel == LEVEL_NONE) {
    if (_state != STATE_RESETTING) {
      DPRINTLN(F("Shutters: level not known, resetting"));
      _up();
      _state = STATE_RESETTING;
      _stateTime = millis();
    } else if (millis() - _stateTime >= _upCourseTime + _upCalibrationTime) {
      DPRINTLN(F("Shutters: level now known"));
      _halt();
      _state = STATE_IDLE;
      _currentLevel = 0;
      _storedState.setLevel(_currentLevel);
      _setStateFunction(this, _storedState.getState());
      _notifyLevel();
    }

    return;
  }

  // here, level is known

  if (_state == STATE_IDLE && _targetLevel == LEVEL_NONE) return; // nothing to do

  if (_state == STATE_CALIBRATING) {
    const uint32_t calibrationTime = (_direction == DIRECTION_UP) ? _upCalibrationTime : _downCalibrationTime;
    if (millis() - _stateTime >= calibrationTime) {
      DPRINTLN(F("Shutters: calibration is done"));
      _halt();
      _state = STATE_IDLE;
      _notifyLevel();
      _setStateFunction(this, _storedState.getState());
    }

    return;
  }

  // here, level is known and calibrated, and we need to do something

  if (_state == STATE_IDLE) {
    DPRINTLN(F("Shutters: starting move"));
    _direction = (_targetLevel > _currentLevel) ? DIRECTION_DOWN : DIRECTION_UP;
    _storedState.setLevel(LEVEL_NONE);
    _setStateFunction(this, _storedState.getState());
    (_direction == DIRECTION_UP) ? _up() : _down();
    _state = STATE_TARGETING;
    _stateTime = millis();

    return;
  }

  // here, we have to handle targeting and normalizing

  const uint32_t stepTime = (_direction == DIRECTION_UP) ? _upStepTime : _downStepTime;

  if (millis() - _stateTime < stepTime) return;

  _currentLevel += (_direction == DIRECTION_UP) ? -1 : 1;
  _storedState.setLevel(_currentLevel);
  _stateTime = millis();

  if (_currentLevel == 0 || _currentLevel == 100) { // we need to calibrate
    DPRINTLN(F("Shutters: starting calibration"));
    _state = STATE_CALIBRATING;
    if (_currentLevel == _targetLevel) _targetLevel = LEVEL_NONE;

    return;
  }

  if (_state == STATE_NORMALIZING) { // we've finished normalizing
    DPRINTLN(F("Shutters: finished normalizing"));
    _halt();
    _state = STATE_IDLE;
    _notifyLevel();
    if (_targetLevel == LEVEL_NONE) _setStateFunction(this, _storedState.getState());

    return;
  }

  if (_state == STATE_TARGETING && _currentLevel == _targetLevel) { // we've reached out target
    DPRINTLN(F("Shutters: reached target"));
    _halt();
    _state = STATE_IDLE;
    _targetLevel = LEVEL_NONE;
    _notifyLevel();
    _setStateFunction(this, _storedState.getState());

    return;
  }

  // we've reached an intermediary level

  _notifyLevel();
}

bool Shutters::isIdle() {
  return _state == STATE_IDLE;
}

uint8_t Shutters::getCurrentLevel() {
  return _currentLevel;
}

void Shutters::reset() {
  _halt();
  _storedState.reset();
  _setStateFunction(this, _storedState.getState());
  _reset = true;
}
