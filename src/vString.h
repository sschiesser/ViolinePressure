#ifndef VSTRING_H
#define VSTRING_H

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "Arduino.h"

#define BUFFER_SIZE 500

class vString
{
  private:
  public:
  char strName;
  bool newVal;
  const uint16_t bufferSize = BUFFER_SIZE;
  uint8_t pin;
  uint16_t adcBuffer[BUFFER_SIZE];
  uint16_t adcHead, adcTail;
  uint16_t adcCurVal;
  uint16_t adcDispValue;
  uint16_t decimalValue;
  struct
  {
    uint16_t min, max;
  } calibRange;

  vString(uint8_t pin); //, char name);
  ~vString();

  uint16_t adcToDecimal();
};

// #ifdef __cplusplus
// }
// #endif

#endif /* VSTRING_H */