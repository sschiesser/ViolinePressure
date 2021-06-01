#ifndef INIT_H
#define INIT_H

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "Arduino.h"

void calibrate();
void doCalibrate(uint8_t adcPin);
void measure();
void measureString(uint8_t adcPin);
// void displayString(uint16_t strVal);
void displayHelp();

enum class MACHINE_STATE : uint8_t {
  IDLE,
  CALIBRATING,
  MEASURING,
  ERROR
};

// #ifdef __cplusplus
// }
// #endif

#endif /* INIT_H */