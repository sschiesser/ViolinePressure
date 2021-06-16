#ifndef INIT_H
#define INIT_H

#include "vString.h"
#include <Arduino.h>
#include <usb_rawhid.h>

enum class HID_MESSAGES : uint8_t {
  COMMAND      = 0xC0,
  CALIB_RANGES = 0xD0,
  CALIB_RANGES_DONE,
  CALIB_TOUCH = 0xE0,
  CALIB_TOUCH_DONE,
  MEASURE = 0xF0,
  END     = 0xFF
};

enum class MACHINE_STATE : uint8_t {
  IDLE         = 0x00,
  CALIB_RANGES = 0x10,
  CALIB_TOUCH  = 0x11,
  MEASURING    = 0x20,
  ERROR        = 0xF0
};

enum class COMMAND_CODES : char {
  STRING_A     = 'a',
  STRING_D     = 'd',
  STRING_E     = 'e',
  STRING_G     = 'g',
  HELP         = 'h',
  MEASURE      = 'm',
  CALIB_RANGES = 'r',
  CALIB_TOUCH  = 't',
  VIEW         = 'v',
  EXIT         = 'x',
  ERR_NOCMD    = (char)0xff,
  ERR_TIMEOUT  = (char)0xfe
};

void parseCommands(COMMAND_CODES cmd);
// void calibrateRange(vString* str);
// void calibrateTouch(vString* str);
// bool doCalibrate(uint8_t adcPin, uint8_t touchPin, range_t* range);
// bool checkCalib();
void measure();
void measureString(uint8_t adcPin);
// void displayRange(range_t range);
// bool saveToEeprom16(uint16_t* value);
// void displayString(uint16_t strVal);
void displayHelp();

#endif /* INIT_H */