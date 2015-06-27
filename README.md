# Arduino Shutters

This Arduino library allows non-smart roller-shutters to be controlled using time.
Using relays, it is easy to make shutters go up and down. But I wanted to be able
to make the shutters go halfway (50%) for example. So I built this lib.

## Features

* Ability to set aperture percentage
* Power outage safe
  * Shutters state saved in EEPROM, completely transparent usage (using 3 bytes)
* Automatic recalibration

## Requirements

* 2 relays
* Measure as precisely as possible the time of a full course in seconds

## Installation

1. Download the [latest version](https://github.com/marvinroger/arduino-shutters/archive/master.zip)
2. Load the `.zip` with **Sketch → Include Library → Add .ZIP Library**

## Hardware


<table>
  <tr>
    <th>Relay</th>
    <th>Active</th>
    <th>Inactive</th>
  </tr>
  <tr>
    <td>Move</td>
    <td>Move</td>
    <td>Don't move</td>
  </tr>
  <tr>
    <td>Direction</td>
    <td>Up</td>
    <td>Down</td>
  </tr>
</table>

## API

See examples folder for examples.

### Shutters (byte pin_move, byte pin_direction, float time_full_course, bool active_low = false, byte eeprom_offset = 0)

* **pin_move**: Arduino Pin on which the move relay is
* **pin_direction**: Arduino Pin on which the direction relay is
* **time_full_course**: Time in seconds to do a full shutters course
* **active_low**: Some relays are LOW active. Default to false (active HIGH)
* **eeprom_offset**: Maybe your code already uses EEPROM, so you can put an offset. Default to 0

### void .begin ()

Setup the shutters. Must be called once in `setup()`.

**Each boot**: Set `pin_move` and `pin_direction` to `OUTPUT`.

**First boot**: Initialize the EEPROM and put the shutters at their minimum position. This will be blocking for `time_full_course` seconds.

**Subsequent boots**: If the Arduino was powered off while the shutters were not moving, nothing is done because the current state of the shutters is known. Else, it will put the shutters at their minimum position and put it back to the last requested state. Might be blocking for a maximum of (`time_full_course` * 2) seconds.

### void .loop ()

Handle the shutters. Must be called in `loop()`. **Don't call `delay()` in loop() as it will block the loop, so Shutters will malfunction.**

### void .requestLevel (byte percentage)

Put the shutters to the given position.
Note that if `percentage` == 0 || `percentage` == 100, the shutters will recalibrate (relays will stay active a bit longer than it should to ensure the shutters are really at their minimum or maximum position).

* **percentage**: Percentage the shutters must go to. If not 0 <= `percentage` <= 100, nothing will be done

### void .stop ()

Stop the shutters.

### bool .isMoving ()

Return whether the shutters are currently moving or not.

### byte .getCurrentLevel ()

Return whether the shutters are currently moving or not. Might be +- 1% if the shutters are moving.
