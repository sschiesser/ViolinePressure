#ifndef INIT_H
#define INIT_H

#include "Arduino.h"
#include "vString.h"

void calibrate();
void doCalibrate(uint8_t adcPin, minmax_t* range);
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