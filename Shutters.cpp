#include "Shutters.h"

using namespace ShuttersInternal;

Shutters::Shutters(uint32_t courseTime, void (*upCallback)(void), void (*downCallback)(void), void (*haltCallback)(void), uint8_t (*getStateCallback)(void), void (*setStateCallback)(uint8_t), float calibrationRatio)
: _courseTime(courseTime)
, _calibrationRatio(calibrationRatio)
, _stepTime(courseTime / LEVELS)
, _calibrationTime(courseTime * calibrationRatio)
, _state(STATE_IDLE)
, _stateTime(0)
, _direction(DIRECTION_UP)
, _level(LEVEL_NONE)
, _targetLevel(LEVEL_NONE)
, _safetyDelay(false)
, _safetyDelayTime(0)
, _reset(false)
, _upCallback(upCallback)
, _downCallback(downCallback)
, _haltCallback(haltCallback)
, _getStateCallback(getStateCallback)
, _setStateCallback(setStateCallback)
{
}

void Shutters::_up() {
  _upCallback();
}

void Shutters::_down() {
  _downCallback();
}

void Shutters::_halt() {
  _haltCallback();
  _setSafetyDelay();
}

void Shutters::_setSafetyDelay() {
  _safetyDelayTime = millis();
  _safetyDelay = true;
}

bool Shutters::begin() {
  _level = _getStateCallback();
}

void Shutters::setLevel(uint8_t level) {
  if (level > 100) {
    return;
  }

  if (_state == STATE_IDLE && level == _level) return;
  if ((_state == STATE_TARGETING || _state == STATE_NORMALIZING) && level == _targetLevel) return; // normalizing check useless, but avoid following lines overhead

  _targetLevel = level;
  Direction direction = _targetLevel > _level ? DIRECTION_DOWN : DIRECTION_UP;
  if (_state == STATE_TARGETING && _direction != direction) {
    _state = STATE_NORMALIZING;
  }
}

void Shutters::stop() {
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

  if (_level == LEVEL_NONE) {
    if (_state != STATE_RESETTING) {
      _up();
      _state = STATE_RESETTING;
      _stateTime = millis();
    } else if (millis() - _stateTime >= _courseTime + _calibrationTime) {
      _halt();
      _state = STATE_IDLE;
      _level = 0;
      _setStateCallback(_level);
    }

    return;
  }

  // here, level is known

  if (_state == STATE_IDLE && _targetLevel == LEVEL_NONE) return; // nothing to do

  if (_state == STATE_CALIBRATING) {
    if (millis() - _stateTime >= _calibrationTime) {
      _halt();
      _state = STATE_IDLE;
      _setStateCallback(_level);
    }

    return;
  }

  // here, level is known and calibrated, and we need to do something

  if (_state == STATE_IDLE) {
    reset();
    _direction = _targetLevel > _level ? DIRECTION_DOWN : DIRECTION_UP;
    _direction == DIRECTION_UP ? _up() : _down();
    _state = STATE_TARGETING;
    _stateTime = millis();

    return;
  }

  // here, we have to handle targeting and normalizing

  if (millis() - _stateTime < _stepTime) return;

  _level += _direction == DIRECTION_UP ? -1 : 1;
  _stateTime = millis();

  if (_level == 0 || _level == 100) {
    _state = STATE_CALIBRATING;
    if (_level == _targetLevel) _targetLevel = LEVEL_NONE;

    return;
  }

  if (_state == STATE_NORMALIZING) {
    _halt();
    _state = STATE_IDLE;
    if (_targetLevel == LEVEL_NONE) _setStateCallback(_level);

    return;
  }

  if (_state == STATE_TARGETING && _level == _targetLevel) {
    _halt();
    _state = STATE_IDLE;
    _targetLevel = LEVEL_NONE;
    _setStateCallback(_level);
  }
}

bool Shutters::isIdle() {
  return _state == STATE_IDLE;
}

uint8_t Shutters::getCurrentLevel() {
  return _level;
}

void Shutters::reset() {
  _reset = true;
  _setStateCallback(LEVEL_NONE);
}
