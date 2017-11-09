# Arduino Shutters

[![Build Status](https://travis-ci.org/marvinroger/arduino-shutters.svg?branch=master)](https://travis-ci.org/marvinroger/arduino-shutters) [![Latest version](https://img.shields.io/github/release/marvinroger/arduino-shutters.svg)](https://github.com/marvinroger/arduino-shutters/releases/latest)

This Arduino library allows non-smart roller-shutters to be percentage-controlled using time.
Using relays, it is easy to make shutters go up and down. But I wanted to be able
to make the shutters go halfway (50%) for example. So I built this library.

## Features

* Ability to set aperture percentage
* Power outage safe
  * Shutters state saved using 20 bytes
  * Store in EEPROM, SPIFFS, etc. using callbacks
* Automatic calibration on extremes (0% and 100%)
* Flexible control method (might use relays, RF, etc.) using callbacks
* Support for multiple shutters

## Requirement

* Measure as precisely as possible the time of a full shutters course
* The initial state callback must return a zeroed char array

## Installation

1. Download the [latest version](https://github.com/marvinroger/arduino-shutters/archive/master.zip)
2. Load the `.zip` with **Sketch → Include Library → Add .ZIP Library**

## API

See examples folder for examples.

#### Shutters (void (\*`upCallback`)(Shutters* shutters), void (\*`downCallback`)(Shutters* shutters), void (\*`haltCallback`)(Shutters* shutters), const char* (\*`getStateCallback`)(Shutters* shutters, byte length), void (\*`setStateCallback`)(Shutters* shutters, const char* state, byte length))

* **`upCallback()`**: Function to execute for the shutters to go up
* **`downCallback()`**: Function to execute for the shutters to go down
* **`haltCallback()`**: Function to execute for the shutters to halt
* **`getStateCallback()`**: Function called to get the stored state. This must return the state char array of the given length
* **`setStateCallback()`**: Function called to set the stored state. Store this in the EEPROM of SPIFFS, etc.

#### Shutters& .begin ()

Setup the shutters.

Must be called once in `setup()`, after `setCourseTime()`, otherwise it won't have any effect.

#### Shutters& .loop ()

Handle the shutters.

Must be called in `loop()`. **Don't call `delay()` in loop() as it will block the loop, so Shutters will malfunction.**

#### unsigned long .getUpCourseTime ()

Returns the up course time or `0` if not yet `begin()`.

#### unsigned long .getDownCourseTime ()

Returns the down course time or `0` if not yet `begin()`.

#### Shutters& .setCourseTime (unsigned long `upCourseTime`, unsigned long `downCourseTime` = 0)

Set the course time. If `downCourseTime` is not set, it will be the same as `upCourseTime`.

Must be called at least one time, before `setup()`. Calling it after `setup()` will have no effect (unless `reset()` has been called).

* **`upCourseTime`**: Up course time, in milliseconds. Must be max 67108864, otherwise it will have no effect
* **`downCourseTime`**: Down course time, in milliseconds. Must be max 67108864, otherwise it will have no effect

#### float .getCalibrationRatio ()

Returns the calibration ration.

#### Shutters& .setCalibrationRatio (float `calibrationRatio` = 0)

Set the course time. If `downCourseTime` is not set, it will be the same as `upCourseTime`.

Can be called anytime, takes effect immediately.

* **`calibrationRatio`**: Calibration ratio. E.g. `0.5` means "50% of the course time". For example, if your course time is `10000`, and the calibration ratio is `0.2`, then, when we `setLevel()` to `0` or `100`, the shutters will move for `0.2 * 10000 = 2000`ms more than normal. That way, we can ensure we are actually at `0` or `100` and thus, that we are calibrated.

#### Shutters& onLevelReached(void (\*`levelReachedCallback`)(Shutters* shutters, byte level))

Set the level reached handler. This handler will be called whenever a new level is reached, along with intermediary levels. E.g. if the levels are at 10% and you request 15%, the callback will be called for 11, 12, 13, 14 and 15%.

Can be called anytime, takes effect immediately.

* **`levelReachedCallback`**: Level reached callback

#### Shutters& .setLevel (byte `percentage`)

Put the shutters in the given position.
Note that if `percentage` == 0 || `percentage` == 100, the shutters will recalibrate (relays will stay active a bit longer than it should to ensure the shutters are really at their minimum or maximum position).

Can only be called after `begin()`, otherwise it won't have any effect.

* **`percentage`**: Percentage the shutters must go to. If not 0 <= `percentage` <= 100, nothing will be done

#### Shutters& .stop ()

Stop the shutters.

Can only be called after `begin()`, otherwise it won't have any effect.

#### bool .isIdle ()

Return whether the shutters are currently idle or not.

#### byte .getCurrentLevel ()

Return the current level of the shutters. Might be +/- 1% if the shutters are moving.

#### Shutters& .reset ()

Erase saved state, for example for a reset routine. This disables the library, until you call `setCourseTime()` and `begin()` again.

#### Shutters& .isReset ()

Return whether the shutters is currently in a reset state or not (e.g. before having called `setCourseTime()` and `begin()`, or after a `reset()`).

## Behavior when changing course time

The stored state does contain the up and down course time, along with whether or not the current level is known, and if so, along with the current level. This means that if you change the course time after boot (or after `reset()`) with a course time that is not the same as the last known one, the shutters will auto-reset. In other words: you don't have to reset the state if you change the course time, everything is handled internally.
