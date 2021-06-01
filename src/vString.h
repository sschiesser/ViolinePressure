#ifndef VSTRING_H
#define VSTRING_H

#include "Arduino.h"

#define BUFFER_SIZE 500
typedef struct
{
  uint16_t min, max;
} minmax_t;

class vString
{
  private:
  public:
  char strName;
  bool newVal;
  const uint16_t bufferSize = BUFFER_SIZE;
  uint8_t adcPin;
  uint16_t adcBuffer[BUFFER_SIZE];
  uint16_t adcHead, adcTail;
  uint16_t adcCurVal;
  uint16_t adcDispValue;
  uint16_t decimalValue;
  uint16_t calMin;
  uint16_t calMax;
  minmax_t calRange;

  vString(uint8_t pin, char name);
  ~vString();

  uint16_t adcToDecimal();
};

#endif /* VSTRING_H */