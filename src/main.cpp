#include "main.h"
#include "vString.h"
#include <ADC.h>
#include <ADC_util.h>
#include <Arduino.h>
#include <EEPROM.h>

elapsedMillis deltaMs;
elapsedMicros deltaUs;
ADC* adc      = new ADC(); // adc object;
vString* Gstr = new vString(GSTR_TOUCH_PIN, GSTR_ADC_PIN, GSTR_NAME_CHAR, (uint8_t)HID_NOTIF::N_STR_G);
vString* Estr = new vString(ESTR_TOUCH_PIN, ESTR_ADC_PIN, ESTR_NAME_CHAR, (uint8_t)HID_NOTIF::N_STR_E);

MACHINE_STATE machineState = MACHINE_STATE::S_IDLE;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  adc->adc0->setAveraging(16);                                            // set number of averages
  adc->adc0->setResolution(16);                                           // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED_16BITS); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);            // change the sampling speed
}

void loop()
{
  HID_REQ req[64];
  uint8_t notif[64];
  static uint16_t eVal, gVal;
  uint8_t n = RawHID.recv(req, 0);
  if (n > 0)
  {
    machineState = parseRequests(req);
  }

  switch (machineState)
  {
    case MACHINE_STATE::S_CT_E: {
      Estr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      Estr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Estr->touchCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      break;
    }

    case MACHINE_STATE::S_CT_G: {
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      Gstr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Gstr->touchCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      break;
    }

    case MACHINE_STATE::S_CR_E: {
      Estr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      Estr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Estr->adcCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      break;
    }

    case MACHINE_STATE::S_CR_G: {
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      Gstr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Gstr->adcCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      break;
    }

    case MACHINE_STATE::S_MEAS: {
      if ((Gstr->adcNewVal) && (Estr->adcNewVal))
      {
        gVal            = Gstr->adcVal;
        eVal            = Estr->adcVal;
        Gstr->adcNewVal = false;
        Estr->adcNewVal = false;
        if (deltaUs > 10)
        {
          notif[0] = (uint8_t)HID_NOTIF::N_MEAS;
          notif[1] = 8;
          notif[2] = ((deltaUs >> 8) & 0xff);
          notif[3] = (deltaUs & 0xff);
          notif[4] = ((gVal >> 8) & 0xff);
          notif[5] = (gVal & 0xff);
          notif[6] = ((eVal >> 8) & 0xff);
          notif[7] = (eVal & 0xff);
          notif[8] = (uint8_t)machineState;
          notif[9] = (uint8_t)HID_NOTIF::N_END;
          RawHID.send(notif, HID_TIMEOUT_MAX);
          deltaUs = 0;
          deltaMs = 0;
        }
      }
      break;
    }

    default:
      break;
  }
}

/* ****************************************************************************
 * MACHINE_STATE parseRequests
 * ----------------------------------------------------------------------------
 * 1. verify if the received request matches the expectet format:
 *    - 1st byte = HID_REQUESTS::R_CMD
 *    - 2nd byte (len) > 0 (at least len byte)
 *    - len+1 byte = HID_REQUESTS::R_END
 * 2. on format match, parse the main command byte (req[2]) and send
 *    acknowledgement notification with corresponding answer
 * 
 * Parameters:
 * - *req           -> received request message
 * 
 * Return:
 * - MACHINE_STATE  -> current machine state induced by the request
 * ****************************************************************************/
MACHINE_STATE parseRequests(HID_REQ* req)
{
  bool msgFormat = false;
  uint8_t len, pos;
  MACHINE_STATE retVal = MACHINE_STATE::S_ERR; //machineState;

  len = (uint8_t)req[1];
  pos = (uint8_t)req[1] + 1;
  if ((req[0] == HID_REQ::R_CMD) &&
      (req[pos] == HID_REQ::R_END) &&
      (len > 0))
  {
    msgFormat = true;
  }

  /* Request format is matching the expectations: we can parse the message */
  if (msgFormat)
  {
    uint8_t notif[HID_PACK_SIZE_MAX] = {0};
    notif[0]                         = (uint8_t)HID_NOTIF::N_INFO;

    switch ((HID_REQ)req[2])
    {
      case HID_REQ::R_MEAS: {
        notif[1] = 3;
        notif[2] = (uint8_t)HID_NOTIF::N_MEAS;
        notif[3] = (uint8_t)machineState;
        notif[4] = (uint8_t)HID_NOTIF::N_END;

        adc->adc0->enableInterrupts(adc0_isr);
        adc->adc0->startSingleRead(Estr->adcPin);

        retVal  = MACHINE_STATE::S_MEAS;
        deltaUs = 0;
        deltaMs = 0;
        break;
      }

      case HID_REQ::R_CALIB_R: {
        notif[1] = 4;
        notif[2] = (uint8_t)HID_NOTIF::N_CALIB_R;
        notif[4] = (uint8_t)machineState;
        notif[5] = (uint8_t)HID_NOTIF::N_END;
        if (req[3] == HID_REQ::R_STR_G)
        {
          notif[3] = (uint8_t)HID_NOTIF::N_STR_G;
          retVal   = MACHINE_STATE::S_CR_G;
        }
        else if (req[3] == HID_REQ::R_STR_E)
        {
          notif[3] = (uint8_t)HID_NOTIF::N_STR_E;
          retVal   = MACHINE_STATE::S_CR_E;
        }
        else
        {
          notif[1] = 3;
          notif[2] = (uint8_t)HID_NOTIF::N_ERR;
          notif[3] = (uint8_t)machineState;
          notif[4] = (uint8_t)HID_NOTIF::N_END;
        }
        break;
      }

      case HID_REQ::R_CALIB_T: {
        notif[1] = 4;
        notif[2] = (uint8_t)HID_NOTIF::N_CALIB_T;
        notif[4] = (uint8_t)machineState;
        notif[5] = (uint8_t)HID_NOTIF::N_END;
        if (req[3] == HID_REQ::R_STR_G)
        {
          notif[3] = (uint8_t)HID_NOTIF::N_STR_G;
          retVal   = MACHINE_STATE::S_CT_G;
        }
        else if (req[3] == HID_REQ::R_STR_E)
        {
          notif[3] = (uint8_t)HID_NOTIF::N_STR_E;
          retVal   = MACHINE_STATE::S_CT_E;
        }
        else
        {
          notif[1] = 3;
          notif[2] = (uint8_t)HID_NOTIF::N_ERR;
          notif[3] = (uint8_t)machineState;
          notif[4] = (uint8_t)HID_NOTIF::N_END;
        }
        break;
      }

      case HID_REQ::R_VIEW: {
        break;
      }

      case HID_REQ::R_EXIT: {
        notif[1] = 3;
        notif[2] = (uint8_t)HID_NOTIF::N_EXIT;
        notif[3] = (uint8_t)machineState;
        notif[4] = (uint8_t)HID_NOTIF::N_END;
        if (machineState == MACHINE_STATE::S_MEAS)
        {
          adc->adc0->disableInterrupts();
          // adc->adc0->stopContinuous();
        }

        retVal = MACHINE_STATE::S_IDLE;
        break;
      }

      default: {
        notif[1] = 3;
        notif[2] = (uint8_t)HID_NOTIF::N_ERR;
        notif[3] = (uint8_t)machineState;
        notif[4] = (uint8_t)HID_NOTIF::N_END;
        break;
      }
    }

    RawHID.send(notif, HID_TIMEOUT_MAX);
  }

  return retVal;
}

/* ****************************************************************************
 * void adc0_isr
 * ----------------------------------------------------------------------------
 * !!! MAKE SURE TO CALL readSingle() TO CLEAR THE INTERRUPT !!!
 * 
 * Check on which pin the ADC channel was set and set the corresponding
 * value & flag. Then start a new singleRead on a next pin.
 * ****************************************************************************/
void adc0_isr()
{
  uint8_t pin = ADC::sc1a2channelADC0[ADC0_SC1A & ADC_SC1A_CHANNELS];
  if (pin == Estr->adcPin)
  {
    if (touchRead(Estr->touchPin) > (int)(1.1 * (float)Estr->touchThresh.min))
    {
      Estr->adcVal = adc->adc0->readSingle();
    }
    else
    {
      Estr->adcVal = 0;
    }
    Estr->adcNewVal = true;
    adc->adc0->startSingleRead(Gstr->adcPin);
  }
  else if (pin == Gstr->adcPin)
  {
    if (touchRead(Gstr->touchPin) > (int)(1.1 * (float)Gstr->touchThresh.min))
    {
      Gstr->adcVal = adc->adc0->readSingle();
    }
    else
    {
      Gstr->adcVal = 0;
    }
    Gstr->adcNewVal = true;
    adc->adc0->startSingleRead(Estr->adcPin);
  }
  else
  {
    adc->readSingle();
  }

  if (adc->adc0->adcWasInUse)
  {
    adc->adc0->loadConfig(&adc->adc0->adc_config);
    adc->adc0->adcWasInUse = false;
  }
}
