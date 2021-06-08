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

void vString::saveToEeprom(enum dataType type, uint8_t* data)
{
  uint8_t eeVals[4];

  switch (type)
  {
    case RANGE:
      uint32_t eepromAddr = EEPROM_RANGE_ADDR * strNumber;
      for (uint8_t i = 0; i < 4; i++)
      {
        eeVals[i] = *data++;
        EEPROM.update((eepromAddr + i), eeVals[i]);
        Serial.printf("eVals[%d] = 0x%02x\n", (eepromAddr + i), eeVals[i]);
      }
      break;

    case TOUCH:
      break;

    case NONE:
    case ERROR:
    default:
      break;
  }
}

void vString::getFromEeprom(enum dataType type, uint8_t* data)
{
  switch (type)
  {
    case RANGE:
      break;

    case TOUCH:
      break;

    case NONE:
    case ERROR:
    default:
      break;
  }
}