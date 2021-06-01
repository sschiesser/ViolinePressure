#include "vString.h"

vString::vString(uint8_t pin, char name, uint8_t number)
{
  adcPin       = pin;
  adcHead      = 0;
  adcTail      = 0;
  strName      = name;
  strNumber    = number;
  calRange.min = UINT16_MAX;
  calRange.max = 0;
  newVal       = false;
  pinMode(adcPin, INPUT);
}

vString::~vString()
{
}