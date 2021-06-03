#ifndef VSTRING_H
#define VSTRING_H

#include "Arduino.h"

#define BUFFER_SIZE 500
#define STR_CALIB_MIN 100
#define STR_CALIB_MAX 800

typedef struct
{
  uint16_t min, max;
} minmax_t;

class vString
{
  private:
  public:
  char strName;
  uint8_t strNumber;
  bool newVal;
  bool calibOK;
  const uint16_t bufferSize = BUFFER_SIZE;
  uint8_t adcPin;
  uint8_t touchPin;
  uint16_t adcBuffer[BUFFER_SIZE];
  uint16_t adcHead, adcTail;
  uint16_t adcCurVal;
  uint16_t adcDispValue;
  uint16_t decimalValue;
  minmax_t calRange;

  vString(uint8_t touchPin, uint8_t adcPin, char name, uint8_t number);
  ~vString();

  bool checkCal();
  bool stringActive(uint8_t touchPin, uint16_t thresh);
};

#endif /* VSTRING_H */