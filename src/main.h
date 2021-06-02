#ifndef INIT_H
#define INIT_H

#include "Arduino.h"
#include "vString.h"

void calibrate();
bool doCalibrate(uint8_t adcPin, minmax_t* range);
bool checkCalib();
void measure();
void measureString(uint8_t adcPin);
void displayRange(minmax_t range);
void displayString(uint16_t strVal);
void displayHelp();

enum class MACHINE_STATE : uint8_t {
  IDLE,
  CALIBRATING,
  MEASURING,
  ERROR
};

#endif /* INIT_H */