#include "vString.h"

vString::vString(uint8_t tPin, uint8_t aPin, char strName, uint8_t strNumber)
{
  touchPin     = tPin;
  adcPin       = aPin;
  bufHead      = 0;
  bufTail      = 0;
  name         = strName;
  number       = strNumber;
  adcRange.min = UINT16_MAX;
  adcRange.max = 0;
  newVal       = false;
  calibOK      = false;
  pinMode(adcPin, INPUT);
}

vString::~vString()
{
}

bool vString::checkCal()
{
  if ((adcRange.min <= STR_CALIB_MIN) && (adcRange.max >= STR_CALIB_MAX))
    calibOK = true;
  else
    calibOK = false;

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

void vString::saveToEeprom(strDataType type, uint8_t* data)
{
  uint8_t eeVals[4];

  switch (type)
  {
    case strDataType::CAL_RANGE: {
      uint32_t eepromAddr = EEPROM_RANGE_ADDR + (number * EEPROM_ADDR_OFFSET);
      for (uint8_t i = 0; i < 4; i++)
      {
        eeVals[i] = *data++;
        EEPROM.update((eepromAddr + i), eeVals[i]);
        Serial.printf("eVals[%d] = 0x%02x\n", (eepromAddr + i), eeVals[i]);
      }
      break;
    }

    case strDataType::CAL_TOUCH: {
      uint32_t eepromAddr = EEPROM_RANGE_ADDR + (number * EEPROM_ADDR_OFFSET);
      break;
    }

    case strDataType::MEAS_RANGE:
    case strDataType::MEAS_TOUCH:
    case strDataType::NONE:
    case strDataType::ERROR:
    default:
      break;
  }
}

void vString::getFromEeprom(strDataType type, uint8_t* data)
{
  switch (type)
  {
    case strDataType::CAL_RANGE: {
      uint32_t eepromAddr = EEPROM_RANGE_ADDR + (number * EEPROM_ADDR_OFFSET);
      break;
    }

    case strDataType::CAL_TOUCH: {
      uint32_t eepromAddr = EEPROM_TOUCH_ADDR + (number * EEPROM_ADDR_OFFSET);
      break;
    }

    case strDataType::MEAS_RANGE:
    case strDataType::MEAS_TOUCH:
    case strDataType::NONE:
    case strDataType::ERROR:
    default:
      break;
  }
}

bool vString::calibrate(strDataType type, ADC_Module* module, minmax_t* range, minmax_t* thresh)
{
  bool doCal          = true;
  bool retVal         = false;
  elapsedMillis delta = 0;

  switch (type)
  {
    case strDataType::CAL_RANGE: {
      range->min = UINT16_MAX;
      range->max = 0;
      while (doCal)
      {
        while (touchRead(touchPin) > 20000)
        {
          uint16_t val = module->analogRead(adcPin);
          if (val > range->max) range->max = val;
          if (val < range->min) range->min = val;
          displayRange(range);
          delta = 0;
        }

        if (delta > 2000)
        {
          Serial.println("Timeout!");
          doCal = false;
        }

        if (Serial.available())
        {
          char c = Serial.read();
          if (c == 'x')
          {
            doCal = false;
            if ((range->min < STR_CALIB_MIN) && (range->max > STR_CALIB_MAX)) retVal = true;
          }
        }
      }

      retVal = checkCal();
      break;
    }

    case strDataType::CAL_TOUCH: {
      thresh->min = UINT16_MAX;
      thresh->max = 0;
      Serial.printf("CAL_TOUCH on ADC %d\n", module->ADC_num);
      break;
    }

    case strDataType::MEAS_RANGE:
    case strDataType::MEAS_TOUCH:
    case strDataType::NONE:
    case strDataType::ERROR:
    default:
      break;
  }

  return retVal;
}

void vString::displayRange(minmax_t* range)
{
  static uint32_t start = 0;
  uint32_t now          = millis();
  if ((now - start) > 100)
  {
    Serial.printf("Range: %d - %d\n", range->min, range->max);
    start = now;
  }
}