#ifndef INIT_H
#define INIT_H

#include "Arduino.h"
#include "vString.h"

void parseCommands(char cmd);
bool waitForCommand(uint16_t timeout);
void calibrateRange();
void calibrateTouch();
// bool doCalibrate(uint8_t adcPin, uint8_t touchPin, range_t* range);
// bool checkCalib();
void measure();
void measureString(uint8_t adcPin);
// void displayRange(range_t range);
// bool saveToEeprom16(uint16_t* value);
// void displayString(uint16_t strVal);
void displayHelp();

enum class MACHINE_STATE : uint8_t {
  IDLE,
  CALIBRATING_RANGE,
  CALIBRATING_TOUCH,
  MEASURING,
  ERROR
};

#endif /* INIT_H */