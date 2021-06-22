#ifndef INIT_H
#define INIT_H

#include "messages.h"
#include "vString.h"
#include <Arduino.h>
#include <usb_rawhid.h>

enum class MACHINE_STATE : uint8_t {
  S_IDLE    = 0x00,
  S_CALIB_R = 0x10,
  S_CR_G    = 0x11,
  S_CR_E    = 0x12,
  S_CALIB_T = 0x20,
  S_CT_G    = 0x21,
  S_CT_E    = 0x22,
  S_MEAS    = 0x50,
  S_ERR     = 0xE0
};

MACHINE_STATE parseRequests(HID_REQ* req);
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