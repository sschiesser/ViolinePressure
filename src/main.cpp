#include "main.h"
#include "vString.h"
#include <ADC.h>
#include <ADC_util.h>
#include <Arduino.h>
#include <EEPROM.h>

// elapsedMillis gDetla, eDelta;
elapsedMillis deltaMs;
elapsedMicros deltaUs;
ADC* adc      = new ADC(); // adc object;
vString* Gstr = new vString(0, A0, 'G', (uint8_t)HID_NOTIF::N_STR_G);
// vString* Dstr = new vString(6, A2, 'D', 1);
// vString* Astr = new vString(4, A3, 'A', 2);
vString* Estr = new vString(1, A1, 'E', (uint8_t)HID_NOTIF::N_STR_E);
// uint8_t buffer[64];
uint32_t start;

MACHINE_STATE machineState = MACHINE_STATE::S_IDLE;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Serial.begin(115200);
  // delay(1000);

  adc->adc0->setAveraging(32);                                         // set number of averages
  adc->adc0->setResolution(10);                                        // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);     // change the sampling speed
}

void loop()
{
  HID_REQ req[64];
  uint8_t notif[64];
  uint16_t eVal, gVal;
  uint8_t n = RawHID.recv(req, 0);
  if (n > 0)
  {
    machineState = parseRequests(req);
  }

  switch (machineState)
  {
    case MACHINE_STATE::S_CT_E:
      Estr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      Estr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Estr->touchCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      break;

    case MACHINE_STATE::S_CT_G:
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      Gstr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Gstr->touchCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      break;

    case MACHINE_STATE::S_CR_E:
      Estr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      Estr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Estr->adcCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      break;

    case MACHINE_STATE::S_CR_G:
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      Gstr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::S_IDLE;
      if (Gstr->adcCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      break;

    case MACHINE_STATE::S_MEAS: {
      gVal     = Gstr->measure(adc->adc0, 10);
      eVal     = Estr->measure(adc->adc0, 10);
      notif[0] = (uint8_t)HID_NOTIF::N_MEAS;
      notif[1] = 8;
      notif[2] = ((deltaMs >> 8) & 0xff);
      notif[3] = (deltaMs & 0xff);
      notif[4] = ((gVal >> 8) & 0xff);
      notif[5] = (gVal & 0xff);
      notif[6] = ((eVal >> 8) & 0xff);
      notif[7] = (eVal & 0xff);
      notif[8] = (uint8_t)machineState;
      notif[9] = (uint8_t)HID_NOTIF::N_END;
      RawHID.send(notif, 2);
      deltaUs = 0;
      deltaMs = 0;
      // delay(2);
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
    uint8_t notif[64] = {0};
    notif[0]          = (uint8_t)HID_NOTIF::N_INFO;

    switch ((HID_REQ)req[2])
    {
      case HID_REQ::R_MEAS: {
        notif[1] = 3;
        notif[2] = (uint8_t)HID_NOTIF::N_MEAS;
        notif[3] = (uint8_t)machineState;
        notif[4] = (uint8_t)HID_NOTIF::N_END;
        retVal   = MACHINE_STATE::S_MEAS;
        deltaUs  = 0;
        deltaMs  = 0;
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
        retVal   = MACHINE_STATE::S_IDLE;
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

    RawHID.send(notif, 50);
  }

  return retVal;
}

//   case HID_REQUESTS::N_STR_E:
//   case HID_REQUESTS::N_STR_G:
//     if (machineState == MACHINE_STATE::R_CALIB_R)
//     {
//       send[1] = (byte)cmd;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//       if (cmd == HID_REQUESTS::N_STR_E)
//         Estr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
//       else
//         Gstr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
//     }
//     else if (machineState == MACHINE_STATE::R_CALIB_T)
//     {
//       send[1] = (byte)cmd;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//       if (cmd == HID_REQUESTS::N_STR_E)
//       {
//         Estr->resetCalibValues(CALIB_TYPE::R_CALIB_T);
//         Estr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Estr->touchThresh);
//         Estr->calibrate(CALIB_TYPE::R_CALIB_T, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
//         if (Estr->touchCalDone) Estr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Estr->touchThresh);
//       }
//       else
//       {
//         Gstr->resetCalibValues(CALIB_TYPE::R_CALIB_T);
//         Gstr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Gstr->touchThresh);
//         Gstr->calibrate(CALIB_TYPE::R_CALIB_T, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
//         if (Gstr->touchCalDone) Gstr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Gstr->touchThresh);
//       }
//     }
//     else
//     {
//       send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     break;

//   case HID_REQUESTS::R_CALIB_R:
//   case HID_REQUESTS::R_CALIB_T:
//     if (machineState == MACHINE_STATE::S_IDLE)
//     {
//       send[1] = (byte)cmd;
//       if (cmd == HID_REQUESTS::R_CALIB_R)
//       {
//         machineState = MACHINE_STATE::R_CALIB_R;
//         send[2]      = (byte)machineState;
//         RawHID.send(send, 64);
//       }
//       if (cmd == HID_REQUESTS::R_CALIB_T)
//       {
//         machineState = MACHINE_STATE::R_CALIB_T;
//         send[2]      = (byte)machineState;
//         RawHID.send(send, 64);
//         // calibrateTouch();
//       }
//     }
//     else
//     {
//       send[1]      = (byte)HID_REQUESTS::ERR_NOCMD;
//       machineState = MACHINE_STATE::S_IDLE;
//       send[2]      = (byte)machineState;
//       RawHID.send(send, 64);
//     }

//     break;

//   case HID_REQUESTS::R_MEAS:
//     if (machineState == MACHINE_STATE::S_IDLE)
//     {
//       send[1]      = (byte)cmd;
//       machineState = MACHINE_STATE::S_MEAS;
//       send[2]      = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     else
//     {
//       send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     break;

//   case HID_REQUESTS::R_HELP:
//     if ((machineState == MACHINE_STATE::S_IDLE) || (machineState == MACHINE_STATE::R_CALIB_R) || (machineState == MACHINE_STATE::R_CALIB_T))
//     {
//       send[1] = (byte)cmd;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     else
//     {
//       send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }

//   case HID_REQUESTS::R_VIEW:
//     if ((machineState == MACHINE_STATE::S_IDLE) || (machineState == MACHINE_STATE::R_CALIB_R) || (machineState == MACHINE_STATE::R_CALIB_T))
//     {
//       send[1]  = (byte)cmd;
//       send[2]  = (byte)((Gstr->adcRange.min >> 8) & 0xff);
//       send[3]  = (byte)(Gstr->adcRange.min & 0xff);
//       send[4]  = (byte)((Gstr->adcRange.max >> 8) & 0xff);
//       send[5]  = (byte)(Gstr->adcRange.max & 0xff);
//       send[6]  = (byte)((Gstr->touchThresh.avg >> 8) & 0xff);
//       send[7]  = (byte)(Gstr->touchThresh.avg & 0xff);
//       send[8]  = (byte)((Estr->adcRange.min >> 8) & 0xff);
//       send[9]  = (byte)(Estr->adcRange.min & 0xff);
//       send[10] = (byte)((Estr->adcRange.max >> 8) & 0xff);
//       send[11] = (byte)(Estr->adcRange.max & 0xff);
//       send[12] = (byte)((Estr->touchThresh.avg >> 8) & 0xff);
//       send[13] = (byte)(Estr->touchThresh.avg & 0xff);
//       send[14] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     else
//     {
//       send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     break;

//   case HID_REQUESTS::R_EXIT:
//     send[1]      = (byte)cmd;
//     machineState = MACHINE_STATE::S_IDLE;
//     send[2]      = (byte)machineState;
//     RawHID.send(send, 64);
//     break;

//   default:
//     send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//     send[2] = (byte)machineState;
//     RawHID.send(send, 64);
//     break;
// }
// }

// void calibrateRange(vString* str)
// {
// if (str->touchCalDone)
// {
// str->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
// str->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&str->adcRange);
// if (str->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &str->adcRange, &str->touchThresh))
// str->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &str->adcRange, &str->touchThresh);
//   Serial.print("Calibration OK! ");
// else
//   Serial.print("Calibration poor! ");

// Serial.printf("Values on E string: 0x%04x - 0x%04x\n", str->adcRange.min, str->adcRange.max);
// if (str->adcCalDone) Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&str->adcRange);
// displayHelp();
// }
// else
// {
//   Serial.println("Please calibrate the string touch thresholds first");
// }
// }

// void calibrateTouch(vString* str)
// {
// char c = 0;

// Serial.println("Calibrate touch thresholds.");
// displayHelp();
// while (machineState == MACHINE_STATE::R_CALIB_T)
// {
//   str->calibrate(CALIB_TYPE::R_CALIB_T, adc->adc0, &str->adcRange, &str->touchThresh);
//   while (!Serial.available())
//     ;

//   c = Serial.read();
//   if (c == 'a')
//   {
//     Serial.println("A string not available yet!");
//   }
//   else if (c == 'd')
//   {
//     Serial.println("D string not available yet!");
//   }
//   else if (c == 'e')
//   {
//     Serial.printf("Calibrating E string...\nTouch the string at different places on different manners ('x' to finish (t: %d, a:%d))\n", Estr->touchPin, Estr->adcPin);
//     Estr->resetCalibValues(CALIB_TYPE::R_CALIB_T);
//     Estr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Estr->touchThresh);
//     if (Estr->calibrate(CALIB_TYPE::R_CALIB_T, adc->adc0, &Estr->adcRange, &Estr->touchThresh))
//       Serial.print("Calibration OK! ");
//     else
//       Serial.print("Calibration poor! ");

//     Serial.printf("Thresholds on E string: 0x%04x - 0x%04x (avg: 0x%04x)\n", Estr->touchThresh.min, Estr->touchThresh.max, Estr->touchThresh.avg);
//     if (Estr->touchCalDone) Estr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Estr->touchThresh);

//     displayHelp();
//   }
//   else if (c == 'g')
//   {
//     Serial.printf("Calibrating G string...\nTouch the string at different places on different manners ('x' to finish (t: %d, a:%d))\n", Gstr->touchPin, Gstr->adcPin);
//     Gstr->resetCalibValues(CALIB_TYPE::R_CALIB_T);
//     Gstr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Gstr->touchThresh);
//     if (Gstr->calibrate(CALIB_TYPE::R_CALIB_T, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh))
//       Serial.print("Calibration OK! ");
//     else
//       Serial.print("Calibration poor! ");

//     Serial.printf("Thresholds on G string: 0x%04x - 0x%04x (avg: 0x%04x)\n", Gstr->touchThresh.min, Gstr->touchThresh.max, Gstr->touchThresh.avg);
//     if (Gstr->touchCalDone) Gstr->saveToEeprom(CALIB_TYPE::R_CALIB_T, (uint8_t*)&Gstr->touchThresh);

//     displayHelp();
//   }
//   else if (c == 'v')
//   {
//     Serial.println("Current calibration values");
//     Gstr->viewCalibValues();
//     Estr->viewCalibValues();
//     displayHelp();
//   }
//   else if (c == 'x')
//   {
//     Serial.println("\nExiting calibration. Values:");
//     Serial.printf("G: adc -> %d - %d     touch -> %d\n", Gstr->adcRange.min, Gstr->adcRange.max, Gstr->touchThresh.avg);
//     Serial.printf("E: adc -> %d - %d     touch -> %d\n", Estr->adcRange.min, Estr->adcRange.max, Estr->touchThresh.avg);
//     machineState = MACHINE_STATE::S_IDLE;
//     displayHelp();
//   }
//   else
//   {
//     Serial.println("Please choose a valid option");
//   }
// }
// }

void measure()
{
  // uint16_t adcVal   = 0;
  // uint16_t touchVal = 0;
  // uint32_t eStart   = millis();
  // uint32_t gStart   = millis();
  // Gstr->adcNewVal   = false;
  // Estr->adcNewVal   = false;

  // // adc->adc0->enableInterrupts(adc0_isr);
  while (machineState == MACHINE_STATE::S_MEAS)
  {
    // Gstr->measure(8);
    //   if ((touchVal = touchRead(Gstr->touchPin)) > 20000)
    //   {
    //     adcVal = adc->adc0->analogRead(Gstr->adcPin);

    //     // if ((Gstr->adcTail + 1) < Gstr->bufferSize)
    //     //   Gstr->adcTail++;
    //     // else
    //     //   Gstr->adcTail = 0;

    //     // adcVal = Gstr->adcBuffer[Gstr->adcTail];
    //     if ((millis() - gStart) > 100)
    //     {
    //       Serial.print(gDetla);
    //       Serial.printf(" G: %d - %d\n", touchVal, adcVal);
    //       gDetla = 0;
    //       gStart = millis();
    //     }
    //   }

    //   if ((touchVal = touchRead(Estr->touchPin)) > 20000)
    //   {
    //     adcVal = adc->adc0->analogRead(Estr->adcPin);

    //     // if ((Estr->adcTail + 1) < Estr->bufferSize)
    //     //   Estr->adcTail++;
    //     // else
    //     //   Estr->adcTail = 0;

    //     // adcVal       = Estr->adcBuffer[Estr->adcTail];
    //     if ((millis() - eStart) > 100)
    //     {
    //       Serial.print(eDelta);
    //       Serial.printf(" E: %d - %d\n", touchVal, adcVal);
    //       eDelta = 0;
    //       eStart = millis();
    //     }
    //   }

    if (Serial.available())
    {
      char c = Serial.read();
      if (c == 'x')
      {
        // adc->adc0->disableInterrupts();
        machineState = MACHINE_STATE::S_IDLE;
        displayHelp();
      }
    }
  }

  //   char c = 0;

  //   while (machineState == MACHINE_STATE::S_MEAS)
  //   {
  //     // Single reads
  //     Gstr->adcValue = measureString(Gstr->pin);
  //     Estr->adcValue = measureString(Estr->pin);

  //     Serial.print("G: ");
  //     displayString(strValues[0]);

  //     Serial.print("E: ");
  //     displayString(strValues[1]);

  //     Serial.println("");
  //     // Print errors, if any.
  //     if (adc->adc0->fail_flag != ADC_ERROR::CLEAR)
  //     {
  //       Serial.print("ADC0: ");
  //       Serial.println(getStringADCError(adc->adc0->fail_flag));
  //     }

  //   }
}

// void displayRange(range_t range)
// {
//   range_t localRange = {.min = UINT16_MAX, .max = 0};
//   uint8_t dispMin     = 0;
//   uint8_t dispMax     = 0;

//   if (range.min < localRange.min)
//   {
//     localRange.min = range.min;
//     dispMin        = 100 * localRange.min / 1023;
//     // Serial.printf("New min: %d\n", localRange.min);
//   }
//   if (range.max > localRange.max)
//   {
//     localRange.max = range.max;
//     dispMax        = 100 * localRange.max / 1023;
//     // Serial.printf("New max: %d\n", localRange.max);
//   }
//   Serial.printf("\r[");
//   for (int i = 0; i < 100; i++)
//   {
//     if (i < dispMin)
//       Serial.printf(" ");
//     else if ((i > dispMin) && (i < dispMax))
//       Serial.printf("*");
//     else if (i > dispMax)
//       Serial.printf(" ");
//   }
//   Serial.printf("]\n");
// }

void displayString(uint16_t strVal)
{
  uint8_t disp = uint8_t(strVal * 99 / adc->adc0->getMaxValue());
  for (int i = 0; i < disp; i++)
  {
    Serial.print("*");
  }
  Serial.println("");
}

void displayHelp()
{
  if (machineState == MACHINE_STATE::S_IDLE)
  {
    Serial.print("'r' : calibrate string pressure RANGE\n't' : calibrate string TOUCH thresholds\n"
                 "'m' : R_MEAS string pressures (continuous)\n'v' : R_VIEW current calibration values\n"
                 "'h' : display R_HELP\n'x' : R_EXIT calibration or measuring modes\n");
  }
  else if ((machineState == MACHINE_STATE::S_CR_G) || (machineState == MACHINE_STATE::S_CT_G))
  {
    Serial.printf("'g': G string\n'e': E string\n'v': R_VIEW current calibrations\n'x': R_EXIT.\n");
  }
}

// If you enable interrupts make sure to call readSingle() to clear the interrupt.
void adc0_isr()
{
  adc->readSingle();

  if (adc->adc0->adcWasInUse)
  {
    // Serial.println("In use");
    adc->adc0->loadConfig(&adc->adc0->adc_config);
    adc->adc0->adcWasInUse = false;
  }
}
