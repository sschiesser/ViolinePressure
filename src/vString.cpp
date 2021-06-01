#include "vString.h"

vString::vString(uint8_t pin) //, char name)
{
  pinMode(pin, INPUT);
  calibRange.min = UINT16_MAX;
  calibRange.max = 0;
  adcHead        = 0;
  adcTail        = 0;
  newVal         = false;
  // strName        = name;
}

vString::~vString()
{
}