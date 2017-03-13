# Arduino Shutters

This Arduino library allows non-smart roller-shutters to be percentage-controlled using time.
Using relays, it is easy to make shutters go up and down. But I wanted to be able
to make the shutters go halfway (50%) for example. So I built this library.

## Features

* Ability to set aperture percentage
* Power outage safe
  * Shutters state saved using 1 byte
  * Store in EEPROM, SPIFFS, etc. using callbacks
* Automatic calibration on extremes (0% and 100%)
* Flexible control method (might use relays, RF, etc.) using callbacks

## Requirement

* Measure as precisely as possible the time of a full shutters course
* The initial state callback must return 255 on first boot

## Installation

1. Download the [latest version](https://github.com/marvinroger/arduino-shutters/archive/master.zip)
2. Load the `.zip` with **Sketch → Include Library → Add .ZIP Library**

## API

See examples folder for examples.

#### Shutters (unsigned long `courseTime`, void (\*`upCallback`)(void), void (\*`downCallback`)(void), void (\*`haltCallback`)(void), byte (\*`getStateCallback`)(void), void (\*`setStateCallback`)(byte state), float `calibrationRatio` = 0.1)

* **`courseTime`**: Time in milliseconds to do a full shutters course
* **`upCallback`**: Function to execute for the shutters to go up
* **`downCallback`**: Function to execute for the shutters to go down
* **`haltCallback`**: Function to execute for the shutters to halt
* **`getStateCallback`**: Function to get state. This must return the state byte, or 255 if you don't know the state byte (on first boot)
* **`setStateCallback(byte state)`**: Function to set the state byte. Store this in the EEPROM of SPIFFS, etc.

#### bool .begin ()

Setup the shutters. Must be called once in `setup()`.

#### void .setLevel (byte `percentage`)

Put the shutters to the given position.
Note that if `percentage` == 0 || `percentage` == 100, the shutters will recalibrate (relays will stay active a bit longer than it should to ensure the shutters are really at their minimum or maximum position).

* **`percentage`**: Percentage the shutters must go to. If not 0 <= `percentage` <= 100, nothing will be done

#### void .stop ()

Stop the shutters.

#### bool .isIdle ()

Return whether the shutters are currently idle or not.

#### void .loop ()

Handle the shutters. Must be called in `loop()`. **Don't call `delay()` in loop() as it will block the loop, so Shutters will malfunction.**

#### byte .getCurrentLevel ()

Return the current level of the shutters. Might be +/- 1% if the shutters are moving.

#### void .reset ()

Erase saved state, for example for a reset routine. This disables the library, so don't forget to restart/reset the Arduino.
