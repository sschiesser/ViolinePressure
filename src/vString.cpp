#include "vString.h"

vString::vString(uint8_t tPin, uint8_t aPin, char name, uint8_t number)
{
  touchPin     = tPin;
  adcPin       = aPin;
  adcHead      = 0;
  adcTail      = 0;
  strName      = name;
  strNumber    = number;
  calRange.min = UINT16_MAX;
  calRange.max = 0;
  newVal       = false;
  calibOK      = false;
  pinMode(adcPin, INPUT);
}

vString::~vString()
{
}

bool vString::checkCal()
{
  return calibOK;
}

bool vString::stringActive(uint8_t touchPin, uint16_t thresh)
{
  uint16_t val = touchRead(touchPin);
  if (val > thresh)
    return true;
  else
    return false;
}