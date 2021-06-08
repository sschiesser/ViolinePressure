#include "vString.h"

vString::vString(uint8_t tPin, uint8_t aPin, char strName, uint8_t strNumber)
{
  name         = strName;
  number       = strNumber;
  adcPin       = aPin;
  adcRange.min = UINT16_MAX;
  adcRange.max = 0;
  adcCalDone   = false;
  adcNewVal    = false;
  pinMode(adcPin, INPUT);

  touchPin        = tPin;
  touchThresh.min = UINT16_MAX;
  touchThresh.avg = UINT16_MAX;
  touchThresh.max = 0;
  touchCalDone    = false;
  pinMode(touchPin, INPUT);
}

vString::~vString()
{
}

void vString::getCalibValues()
{
  getFromEeprom(strDataType::CAL_RANGE);
  if ((adcRange.min < ADC_RANGE_MIN) && (adcRange.max > ADC_RANGE_MAX)) adcCalDone = true;

  getFromEeprom(strDataType::CAL_TOUCH);
  if ((touchThresh.avg > TOUCH_THRESH_MIN) && (touchThresh.avg < TOUCH_THRESH_MAX)) touchCalDone = true;

  viewCalibValues();
}

bool vString::checkCalStatus(strDataType type)
{
  bool retVal = false;

  switch (type)
  {
    case strDataType::CAL_RANGE: {
      if ((adcRange.min <= ADC_RANGE_MIN) && (adcRange.max >= ADC_RANGE_MAX))
        adcCalDone = true;
      else
        adcCalDone = false;

      retVal = adcCalDone;
      break;
    }

    case strDataType::CAL_TOUCH: {
      if ((touchThresh.avg > TOUCH_THRESH_MIN) && (touchThresh.avg < TOUCH_THRESH_MAX))
        touchCalDone = true;
      else
        touchCalDone = false;

      retVal = touchCalDone;
      break;
    }

    case strDataType::MEAS_TOUCH:
    case strDataType::MEAS_RANGE:
    case strDataType::ERROR:
    case strDataType::NONE:
    default:
      break;
  }

  return retVal;
}

void vString::saveToEeprom(strDataType type, uint8_t* data)
{
  switch (type)
  {
    case strDataType::CAL_RANGE: {
      uint8_t len         = 4;
      uint32_t eepromAddr = EEPROM_RANGE_ADDR + (number * EEPROM_ADDR_OFFSET);
      for (uint8_t i = 0; i < len; i++)
      {
        EEPROM.update((eepromAddr + i), *data);
        Serial.printf("eVals[%d] = 0x%02x\n", (eepromAddr + i), *data);
        data += 1;
      }
      break;
    }

    case strDataType::CAL_TOUCH: {
      uint8_t len         = 6;
      uint32_t eepromAddr = EEPROM_TOUCH_ADDR + (number * EEPROM_ADDR_OFFSET);
      for (uint8_t i = 0; i < len; i++)
      {
        EEPROM.update((eepromAddr + i), *data);
        Serial.printf("eeVals[%d] = 0x%02x\n", (eepromAddr + i), *data);
        data += 1;
      }
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

void vString::getFromEeprom(strDataType type)
{
  switch (type)
  {
    case strDataType::CAL_RANGE: {
      uint32_t eepromAddr = EEPROM_RANGE_ADDR + (number * EEPROM_ADDR_OFFSET);
      uint8_t data[4];
      for (uint8_t i = 0; i < 4; i++)
      {
        data[i] = EEPROM.read(eepromAddr + i);
      }
      adcRange.min = ((uint16_t)data[0] & 0x00ff) | (((uint16_t)data[1] << 8) & 0xff00);
      adcRange.max = ((uint16_t)data[2] & 0x00ff) | (((uint16_t)data[3] << 8) & 0xff00);
      break;
    }

    case strDataType::CAL_TOUCH: {
      uint32_t eepromAddr = EEPROM_TOUCH_ADDR + (number * EEPROM_ADDR_OFFSET);
      uint8_t data[6];
      for (uint8_t i = 0; i < 6; i++)
      {
        data[i] = EEPROM.read(eepromAddr + i);
      }
      touchThresh.min = ((uint16_t)data[0] & 0x00ff) | (((uint16_t)data[1] << 8) & 0xff00);
      touchThresh.max = ((uint16_t)data[2] & 0x00ff) | (((uint16_t)data[3] << 8) & 0xff00);
      touchThresh.avg = ((uint16_t)data[4] & 0x00ff) | (((uint16_t)data[5] << 8) & 0xff00);
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

bool vString::calibrate(strDataType type, ADC_Module* module, range_t* range, thresh_t* thresh)
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
        while (touchRead(touchPin) > touchThresh.avg)
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
          if (c == 'x') doCal = false;
        }
      }

      retVal = checkCalStatus(strDataType::CAL_RANGE);
      break;
    }

    case strDataType::CAL_TOUCH: {
      thresh->min = UINT16_MAX;
      thresh->max = 0;
      while (doCal)
      {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < UINT8_MAX; i++)
        {
          touchBuf[i] = touchRead(touchPin);
          sum += touchBuf[i];
        }
        uint32_t avg = (uint32_t)(sum / UINT8_MAX);
        if (avg > thresh->max) thresh->max = avg;
        if (avg < thresh->min) thresh->min = avg;
        Serial.print(".");

        if (Serial.available())
        {
          char c = Serial.read();
          if (c == 'x')
          {
            doCal       = false;
            thresh->avg = (thresh->max + thresh->min) / 2;
            retVal      = checkCalStatus(strDataType::CAL_TOUCH);
          }
        }
      }
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

void vString::displayRange(range_t* range)
{
  static uint32_t start = 0;
  uint32_t now          = millis();
  if ((now - start) > 100)
  {
    Serial.printf("Range: %d - %d\n", range->min, range->max);
    start = now;
  }
}

void vString::displayTouch(thresh_t* thresh)
{
  static uint32_t start = 0;
  uint32_t now          = millis();
  if ((now - start) > 100)
  {
    Serial.printf("Thresholds: %d - %d\n", thresh->min, thresh->max);
    start = now;
  }
}

void vString::viewCalibValues()
{
  Serial.printf("String name: %c, ADC range: %d - %d, touch thresholds: %d - %d (%d)\n", name, adcRange.min, adcRange.max, touchThresh.min, touchThresh.max, touchThresh.avg);
}

void vString::viewStringValues()
{
  Serial.printf("String name: %c, number: %d, ADC pin: %d, touch pin: %d\n", name, number, adcPin, touchPin);
}

void vString::resetCalibValues(strDataType type)
{
  switch (type)
  {
    case strDataType::CAL_RANGE: {
      adcCalDone   = false;
      adcRange.min = UINT16_MAX;
      adcRange.max = 0;
      break;
    }

    case strDataType::CAL_TOUCH: {
      touchCalDone    = false;
      touchThresh.min = UINT16_MAX;
      touchThresh.avg = UINT16_MAX;
      touchThresh.max = 0;
      break;
    }

    case strDataType::MEAS_RANGE:
    case strDataType::MEAS_TOUCH:
    case strDataType::ERROR:
    case strDataType::NONE:
    default:
      break;
  }
}