# Arduino Shutters

This Arduino library allows non-smart roller-shutters to be controlled using time.
Using relays, it is easy to make shutters go up and down. But I wanted to be able
to make the shutters go halfway (50%) for example. So I built this lib.

## Features

* Ability to set aperture percentage
* Power outage safe
  * Shutters state saved in EEPROM (using 1 byte)
    * ESP8266 compatible
* Automatic recalibration
* Flexible control method (might use relays, RF...)

## Requirement

* Measure as precisely as possible the time of a full course in seconds

## Notes

#### EEPROM lifetime

An Arduino EEPROM cell (byte) is rated for 100 000 erase cycles.
This library is optimized to use the EEPROM as little as possible.
Each time you use the `requestLevel` or `stop` functions, 1 erase happens.
To illustrate that, let's say you open and close your shutters 4 times a day.
The EEPROM won't die until at least `100 000/365/4 ~= 68.5` years — this should be enough, nope? —.

## Installation

1. Download the [latest version](https://github.com/marvinroger/arduino-shutters/archive/master.zip)
2. Load the `.zip` with **Sketch → Include Library → Add .ZIP Library**
3. Be sure that the EEPROM byte you want to use (0 by default) is clear.
You can load the EraseEEPROM example sketch to achieve this

## API

See examples folder for examples.

#### Shutters (float `time_full_course`, void (\*`upCallback`)(void), void (\*`downCallback`)(void), void (\*`haltCallback`)(void), byte `eeprom_offset` = 0)

* **`time_full_course`**: Time in seconds to do a full shutters course
* **`upCallback`**: Function to execute for the shutters to go up
* **`downCallback`**: Function to execute for the shutters to go down
* **`haltCallback`**: Function to execute for the shutters to halt
* **`eeprom_offset`**: Maybe your code already uses EEPROM, so you can put an offset. Default to 0

#### bool .begin ()

Setup the shutters. Must be called once in `setup()`.
Return: read below.

**First boot**: Initialize the EEPROM and put the shutters at their minimum position. This will be blocking for `time_full_course` seconds. Return true.

**Subsequent boots**: If the Arduino was powered off while the shutters were not moving, nothing is done because the current state of the shutters is known and false is returned. Else, it will put the shutters at their minimum position so the level will be known again (being 0%), blocking for `time_full_course` seconds and true will be returned.

#### void .loop ()

Handle the shutters. Must be called in `loop()`. **Don't call `delay()` in loop() as it will block the loop, so Shutters will malfunction.**

#### void .requestLevel (byte `percentage`)

Put the shutters to the given position.
Note that if `percentage` == 0 || `percentage` == 100, the shutters will recalibrate (relays will stay active a bit longer than it should to ensure the shutters are really at their minimum or maximum position).

* **`percentage`**: Percentage the shutters must go to. If not 0 <= `percentage` <= 100, nothing will be done

#### void .stop ()

Stop the shutters.

#### bool .moving ()

Return whether the shutters are currently moving or not.

#### byte .currentLevel ()

Return the current level of the shutters. Might be +/- 1% if the shutters are moving.

#### void .eraseSavedState ()

Erase data stored in EEPROM, for example for a reset routine.
Once erased, don't forget to restart/reset the Arduino, otherwise EEPROM might
be rewritten if `requestLevel` is called for example.
