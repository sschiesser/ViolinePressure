#include "vString.h"

vString::vString(uint8_t pin, char name)
{
  adcPin  = pin;
  adcHead = 0;
  adcTail = 0;
  strName = name;
  calMin  = UINT16_MAX;
  calMax  = 0;
  // calibRange.min = UINT16_MAX;
  // calibRange.max = 0;
  newVal = false;
  pinMode(adcPin, INPUT);
}

vString::~vString()
{
}