#include "vString.h"
#include "main.h"

/******************************************************************************
 * vString::vString
 * ----------------------------------------------------------------------------
 * Class object constructor
 ******************************************************************************/
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

/******************************************************************************
 * vString::~vString
 * ----------------------------------------------------------------------------
 * Class object destructor
 ******************************************************************************/
vString::~vString()
{
}

/******************************************************************************
 * vString::getCalibValues
 * ----------------------------------------------------------------------------
 ******************************************************************************/
void vString::getCalibValues()
{
  getFromEeprom(CALIB_TYPE::CALIB_RANGE);
  if ((adcRange.min < ADC_RANGE_MIN) && (adcRange.max > ADC_RANGE_MAX)) adcCalDone = true;

  getFromEeprom(CALIB_TYPE::CALIB_TOUCH);
  if ((touchThresh.avg > TOUCH_THRESH_MIN) && (touchThresh.avg < TOUCH_THRESH_MAX)) touchCalDone = true;

  // viewCalibValues();
}

/******************************************************************************
 * vString::checkCalStatus
 * ----------------------------------------------------------------------------
 ******************************************************************************/
bool vString::checkCalStatus(CALIB_TYPE type)
{
  bool retVal = false;

  switch (type)
  {
    case CALIB_TYPE::CALIB_RANGE: {
      if ((adcRange.min <= ADC_RANGE_MIN) && (adcRange.max >= ADC_RANGE_MAX))
        adcCalDone = true;
      else
        adcCalDone = false;

      retVal = adcCalDone;
      break;
    }

    case CALIB_TYPE::CALIB_TOUCH: {
      if ((touchThresh.avg > TOUCH_THRESH_MIN) && (touchThresh.avg < TOUCH_THRESH_MAX))
        touchCalDone = true;
      else
        touchCalDone = false;

      retVal = touchCalDone;
      break;
    }

    case CALIB_TYPE::CALIB_NONE:
    default:
      break;
  }

  return retVal;
}

/******************************************************************************
 * vString::saveToEeprom
 * ----------------------------------------------------------------------------
 ******************************************************************************/
void vString::saveToEeprom(CALIB_TYPE type, uint8_t* data)
{
  switch (type)
  {
    case CALIB_TYPE::CALIB_RANGE: {
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

    case CALIB_TYPE::CALIB_TOUCH: {
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

    case CALIB_TYPE::CALIB_NONE:
    default:
      break;
  }
}

/******************************************************************************
 * vString::getFromEeprom
 * ----------------------------------------------------------------------------
 ******************************************************************************/
void vString::getFromEeprom(CALIB_TYPE type)
{
  switch (type)
  {
    case CALIB_TYPE::CALIB_RANGE: {
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

    case CALIB_TYPE::CALIB_TOUCH: {
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

    case CALIB_TYPE::CALIB_NONE:
    default:
      break;
  }
}

/******************************************************************************
 * vString::calibrate
 * ----------------------------------------------------------------------------
 * Perform a *blocking* calibration loop on given parameters.
 * 
 * Parameters:
 * - type     ->  calibration type: CALIB_RANGE (min/max of ADC conversion) and
 *                CALIB_TOUCH (min/max/avg of string capacitive touch threshold)
 * - *module  ->  ADC module to work with
 * - *range   ->  min/max struct to save CALIB_RANGE values
 * - *thresh  ->  min/max/avg struct to save CALIB_TOUCH values
 * 
 * Return:
 * - bool     ->  calibration values are in an acceptable range
 ******************************************************************************/
bool vString::calibrate(CALIB_TYPE type, ADC_Module* module, range_t* range, thresh_t* thresh)
{
  bool doCal          = true;
  bool retVal         = false;
  uint8_t send[64]    = {0};
  uint8_t recv[64]    = {0};
  elapsedMillis delta = 0;

  switch (type)
  {
    /* CALIB_TYPE::CALIB_RANGE
     * -  Read ADC values as long as the string is touched (i.e. above the
     *    previously calibrated threshold), determine min/max values and
     *    send notification.
     * -  If the string has not been touched for 2 seconds, stop reading,
     *    check calibration values and send final notification.
     */
    case CALIB_TYPE::CALIB_RANGE: {
      range->min = UINT16_MAX;
      range->max = 0;
      while (doCal)
      {
        while (touchRead(touchPin) > touchThresh.avg)
        {
          uint16_t val = module->analogRead(adcPin);
          if (val > range->max) range->max = val;
          if (val < range->min) range->min = val;

          send[0] = (uint8_t)HID_NOTIFICATIONS::CALIB_RANGES;
          send[1] = 6;
          send[2] = ((range->min >> 8) & 0xff);
          send[3] = (range->min) & 0xff;
          send[4] = ((range->max >> 8) & 0xff);
          send[5] = (range->max) & 0xff;
          send[6] = (uint8_t)MACHINE_STATE::CALIB_RANGES;
          send[7] = (uint8_t)HID_NOTIFICATIONS::END;
          RawHID.send(send, 64);

          delta = 0;
        } // while(touchRead())

        if (delta > 2000)
        {
          doCal = false;
        }
      } // while(doCal)

      retVal = checkCalStatus(CALIB_TYPE::CALIB_RANGE);

      send[0] = (uint8_t)HID_NOTIFICATIONS::CALIB_RANGES_DONE;
      send[1] = 7;
      send[2] = ((range->min >> 8) & 0xff);
      send[3] = (range->min & 0xff);
      send[4] = ((range->max >> 8) & 0xff);
      send[5] = (range->max & 0xff);
      send[6] = retVal;
      send[7] = (uint8_t)MACHINE_STATE::CALIB_RANGES;
      send[8] = (uint8_t)HID_NOTIFICATIONS::END;
      RawHID.send(send, 64);

      break;
    }

    /* CALIB_TYPE::CALIB_TOUCH
     * -  Read & average capacitive value on the given string, determine
     *    min/max values and send notification.
     * -  If 'x' (EXIT) is received, stop reading, calculate the average
     *    threshold, check if it is in an acceptable range and send
     *    the final calibration values
     */
    case CALIB_TYPE::CALIB_TOUCH: {
      thresh->min = UINT16_MAX;
      thresh->max = 0;
      while (doCal)
      {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < 50; i++)
        {
          touchBuf[i] = touchRead(touchPin);
          sum += touchBuf[i];
        }
        uint16_t avg = (uint16_t)(sum / 50);
        if (avg > thresh->max) thresh->max = avg;
        if (avg < thresh->min) thresh->min = avg;

        send[0]           = (uint8_t)HID_NOTIFICATIONS::CALIB_TOUCH;
        send[1]           = 6;
        send[2]           = ((thresh->min >> 8) & 0xff);
        send[3]           = (thresh->min & 0xff);
        send[4]           = ((thresh->max >> 8) & 0xff);
        send[5]           = (thresh->max & 0xff);
        send[6]           = (uint8_t)MACHINE_STATE::CALIB_TOUCH;
        send[send[1] + 1] = (uint8_t)HID_NOTIFICATIONS::END;
        RawHID.send(send, 64);

        uint8_t n = RawHID.recv(recv, 0);
        if (n > 0)
        {
          uint8_t len = recv[1];
          uint8_t pos = recv[1] + 1;
          if ((recv[0] == (uint8_t)HID_REQUESTS::REQUEST) &&
              (recv[pos] == (uint8_t)HID_REQUESTS::END) &&
              (recv[2] == (uint8_t)HID_REQUESTS::EXIT) &&
              len > 0)
          {
            doCal       = false;
            thresh->avg = (thresh->max + thresh->min) / 2;
            retVal      = checkCalStatus(CALIB_TYPE::CALIB_TOUCH);
          }
        }
      } // while(doCal)

      send[0]           = (uint8_t)HID_NOTIFICATIONS::CALIB_TOUCH_DONE;
      send[1]           = 8;
      send[2]           = ((thresh->min >> 8) & 0xff);
      send[3]           = (thresh->min & 0xff);
      send[4]           = ((thresh->max >> 8) & 0xff);
      send[5]           = (thresh->max & 0xff);
      send[6]           = ((thresh->avg >> 8) & 0xff);
      send[7]           = (thresh->avg & 0xff);
      send[8]           = retVal;
      send[9]           = (uint8_t)MACHINE_STATE::CALIB_TOUCH_E;
      send[send[1] + 1] = (uint8_t)HID_NOTIFICATIONS::END;
      RawHID.send(send, 64);
      break;
    }

    case CALIB_TYPE::CALIB_NONE:
    default:
      break;
  }

  return retVal;
}

/*****************************************************************************
 * vString::measure
 * ---------------------------------------------------------------------------
 * Perform & return the ADC measurement value if the string is touched,
 * otherwise return 0.
 *
 * Parameters:
 * - *module    -> ADC module to work with
 * - smoothing  -> smoothing value (not used yet)
 * 
 * Return:
 * uint16_t     -> measured ADC value (8 - 16 bit)
 *****************************************************************************/
uint16_t vString::measure(ADC_Module* module, uint8_t smoothing)
{
  if (touchRead(touchPin) > (int)(1.1 * (float)touchThresh.min))
    return module->analogRead(adcPin);
  else
    return 0;
}

// void vString::displayRange(range_t* range)
// {
//   static uint32_t start = 0;
//   uint32_t now          = millis();
//   if ((now - start) > 100)
//   {
//     Serial.printf("Range: %d - %d\n", range->min, range->max);
//     start = now;
//   }
// }

// void vString::displayTouch(thresh_t* thresh)
// {
//   static uint32_t start = 0;
//   uint32_t now          = millis();
//   if ((now - start) > 100)
//   {
//     Serial.printf("Thresholds: %d - %d\n", thresh->min, thresh->max);
//     start = now;
//   }
// }

// void vString::viewCalibValues()
// {
//   Serial.printf("String name: %c, ADC range: %d - %d, touch thresholds: %d - %d (%d)\n", name, adcRange.min, adcRange.max, touchThresh.min, touchThresh.max, touchThresh.avg);
// }

// void vString::viewStringValues()
// {
//   Serial.printf("String name: %c, number: %d, ADC pin: %d, touch pin: %d\n", name, number, adcPin, touchPin);
// }

/******************************************************************************
 * vString::resetCalibValues
 * ----------------------------------------------------------------------------
 ******************************************************************************/
void vString::resetCalibValues(CALIB_TYPE type)
{
  switch (type)
  {
    case CALIB_TYPE::CALIB_RANGE: {
      adcCalDone   = false;
      adcRange.min = UINT16_MAX;
      adcRange.max = 0;
      break;
    }

    case CALIB_TYPE::CALIB_TOUCH: {
      touchCalDone    = false;
      touchThresh.min = UINT16_MAX;
      touchThresh.avg = UINT16_MAX;
      touchThresh.max = 0;
      break;
    }

    case CALIB_TYPE::CALIB_NONE:
    default:
      break;
  }
}