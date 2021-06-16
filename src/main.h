#ifndef INIT_H
#define INIT_H

#include "vString.h"
#include <Arduino.h>
#include <usb_rawhid.h>

enum class HID_REQUESTS : uint8_t {
  // request type headers (< 0x20)
  COMMAND = 0x10,
  // requests values (0x20 <= val <= 0x7E)
  STRING_A     = 'a',
  STRING_D     = 'd',
  STRING_E     = 'e',
  STRING_G     = 'g',
  STRING_NONE  = 0xe0,
  HELP         = 'h',
  MEASURE      = 'm',
  CALIB_RANGES = 'r',
  CALIB_TOUCH  = 't',
  VIEW         = 'v',
  EXIT         = 'x',
  // request end delimiter
  END = 0xFF,
};

enum class HID_NOTIFICATIONS : uint8_t {
  // notification type headers
  MEASUREMENT     = 0x00,
  ACKNOWLEDGEMENT = 0x10,
  // notification values
  CALIB_RANGES      = 0x20,
  CALIB_RANGES_DONE = 0x21,
  CALIB_TOUCH       = 0x30,
  CALIB_TOUCH_DONE  = 0x31,
  MEASURE_REQ       = 0x40,
  VIEW_VALUES       = 0x50,
  STRING_E          = 0x60,
  STRING_G          = 0x61,
  STRING_D          = 0x62,
  STRING_A          = 0x63,
  ERROR_BADCMD      = 0xE0,
  ERROR_TIMEOUT     = 0xE1,
  // notification end delimiter
  END = 0xFF
};

enum class MACHINE_STATE : uint8_t {
  IDLE           = 0x00,
  CALIB_RANGES   = 0x10,
  CALIB_RANGES_G = 0x11,
  CALIB_RANGES_E = 0x12,
  CALIB_TOUCH    = 0x20,
  CALIB_TOUCH_G  = 0x21,
  CALIB_TOUCH_E  = 0x22,
  MEASURING      = 0x50,
  ERROR          = 0xE0
};

MACHINE_STATE parseRequests(HID_REQUESTS* req);
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