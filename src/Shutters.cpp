#include "Shutters.hpp"

using namespace ShuttersInternal;

Shutters::Shutters(OperationFunction up, OperationFunction down, OperationFunction halt, GetStateFunction getState, SetStateFunction setState)
: _upCourseTime(0)
, _downCourseTime(0)
, _calibrationRatio(0.1)
, _state(STATE_IDLE)
, _stateTime(0)
, _direction(DIRECTION_UP)
, _storedState()
, _targetLevel(LEVEL_NONE)
, _safetyDelay(false)
, _safetyDelayTime(0)
, _reset(true)
, _upFunction(up)
, _downFunction(down)
, _haltFunction(halt)
, _getStateFunction(getState)
, _setStateFunction(setState)
, _levelReachedCallback(nullptr)
{
  _storedState.feed(_getStateFunction(this));
  if (_storedState.isValid()) _notifyLevel();
}

void Shutters::_up() {
  _upFunction(this);
}

void Shutters::_down() {
  _downFunction(this);
}

void Shutters::_halt() {
  _haltFunction(this);
  _setSafetyDelay();
}

void Shutters::_setSafetyDelay() {
  _safetyDelayTime = millis();
  _safetyDelay = true;
}

void Shutters::_notifyLevel() {
  if (_levelReachedCallback) _levelReachedCallback(this, _storedState.getLevel());
}

uint32_t Shutters::getUpCourseTime() {
  return _upCourseTime;
}

uint32_t Shutters::getDownCourseTime() {
  return _downCourseTime;
}


void Shutters::setCourseTime(uint32_t upCourseTime, uint32_t downCourseTime) {
  if (!_reset) return;

  if (upCourseTime > 67108864UL|| upCourseTime == 0) return; // max value for 26 bits

  if (upCourseTime != _storedState.getUpCourseTime()) {
    _storedState.setLevel(LEVEL_NONE);
    _setStateFunction(this, _storedState.getState());
  }
  _upCourseTime = upCourseTime;

  _upStepTime = upCourseTime / LEVELS;
  _upCalibrationTime = upCourseTime * _calibrationRatio;

  _storedState.setUpCourseTime(upCourseTime);
  _setStateFunction(this, _storedState.getState());

  // if down course time is not set, consider it's the same as up
  if (downCourseTime == 0) downCourseTime = upCourseTime;

  if (downCourseTime > 67108864UL) return; // max value for 26 bits

  if (downCourseTime != _storedState.getDownCourseTime()) {
    _storedState.setLevel(LEVEL_NONE);
    _setStateFunction(this, _storedState.getState());
  }
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
  if (_reset) return;

  if (level > 100) {
    return;
  }

  if (_state == STATE_IDLE && level == _storedState.getLevel()) return;
  if ((_state == STATE_TARGETING || _state == STATE_NORMALIZING) && level == _targetLevel) return; // normalizing check useless, but avoid following lines overhead

  _targetLevel = level;
  Direction direction = _targetLevel > _storedState.getLevel() ? DIRECTION_DOWN : DIRECTION_UP;
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
      _safetyDelay = false;
    }

    return;
  }

  // here, we're safe for relays

  if (_storedState.getLevel() == LEVEL_NONE) {
    if (_state != STATE_RESETTING) {
      _up();
      _state = STATE_RESETTING;
      _stateTime = millis();
    } else if (millis() - _stateTime >= _upCourseTime + _upCalibrationTime) {
      _halt();
      _state = STATE_IDLE;
      _storedState.setLevel(0);
      _setStateFunction(this, _storedState.getState());
      _notifyLevel();
    }

    return;
  }

  // here, level is known

  if (_state == STATE_IDLE && _targetLevel == LEVEL_NONE) return; // nothing to do

  if (_state == STATE_CALIBRATING) {
    const uint32_t calibrationTime = _direction == DIRECTION_UP ? _upCalibrationTime : _downCalibrationTime;
    if (millis() - _stateTime >= calibrationTime) {
      _halt();
      _state = STATE_IDLE;
      _notifyLevel();
      _setStateFunction(this, _storedState.getState());
    }

    return;
  }

  // here, level is known and calibrated, and we need to do something

  if (_state == STATE_IDLE) {
    _storedState.setLevel(LEVEL_NONE);
    _setStateFunction(this, _storedState.getState());
    _direction = _targetLevel > _storedState.getLevel() ? DIRECTION_DOWN : DIRECTION_UP;
    _direction == DIRECTION_UP ? _up() : _down();
    _state = STATE_TARGETING;
    _stateTime = millis();

    return;
  }

  // here, we have to handle targeting and normalizing

  const uint32_t stepTime = _direction == DIRECTION_UP ? _upStepTime : _downStepTime;

  if (millis() - _stateTime < stepTime) return;

  _storedState.setLevel(_storedState.getLevel() + (_direction == DIRECTION_UP ? -1 : 1));
  _stateTime = millis();

  if (_storedState.getLevel() == 0 || _storedState.getLevel() == 100) { // we need to calibrate
    _state = STATE_CALIBRATING;
    if (_storedState.getLevel() == _targetLevel) _targetLevel = LEVEL_NONE;

    return;
  }

  if (_state == STATE_NORMALIZING) { // we've finished normalizing
    _halt();
    _state = STATE_IDLE;
    _notifyLevel();
    if (_targetLevel == LEVEL_NONE) _setStateFunction(this, _storedState.getState());

    return;
  }

  if (_state == STATE_TARGETING && _storedState.getLevel() == _targetLevel) { // we've reached out target
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
  return _storedState.getLevel();
}

void Shutters::reset() {
  _halt();
  _storedState.reset();
  _setStateFunction(this, _storedState.getState());
  _reset = true;
}
