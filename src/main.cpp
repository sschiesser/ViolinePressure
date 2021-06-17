#include "main.h"
#include "vString.h"
#include <ADC.h>
#include <ADC_util.h>
#include <Arduino.h>
#include <EEPROM.h>

elapsedMillis gDetla, eDelta;
ADC* adc      = new ADC(); // adc object;
vString* Gstr = new vString(0, A0, 'G', 0);
// vString* Dstr = new vString(6, A2, 'D', 1);
// vString* Astr = new vString(4, A3, 'A', 2);
vString* Estr = new vString(1, A1, 'E', 1);
uint8_t buffer[64];
uint32_t start;

MACHINE_STATE machineState = MACHINE_STATE::IDLE;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(1000);

  adc->adc0->setAveraging(8);                                          // set number of averages
  adc->adc0->setResolution(10);                                        // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);     // change the sampling speed

  Gstr->viewStringValues();
  Gstr->getCalibValues();
  Serial.println("");
  Estr->viewStringValues();
  Estr->getCalibValues();
  Serial.println("");

  displayHelp();
}

/* Packets sent from host to Teensy (requests)
    - request len req1 req2 req3 ... end
*/
/* Packets sent from Teensy to host (notifications)
    - acknowledgement len ack1 ack2 ack3 ... m_state end
      OR
    - measurement len meas1 meas2 meas3 ... end measurement values should follow the scheme:
      '0x00 len TtimeH TtimeL StimeH StimeL Str1H Str1L Str2H Str2L Str3H Str3L Str4H Str4L 0xff'
*/
void loop()
{

  HID_REQUESTS req[64];
  uint8_t n = RawHID.recv(req, 0);
  if (n > 0)
  {
    machineState = parseRequests(req);
  }

  switch (machineState)
  {
    case MACHINE_STATE::CALIB_TOUCH_E:
      Estr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      Estr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::IDLE;
      if (Estr->touchCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
      break;

    case MACHINE_STATE::CALIB_TOUCH_G:
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      Gstr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::IDLE;
      if (Gstr->touchCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
      break;

    case MACHINE_STATE::CALIB_RANGES_E:
      Estr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      Estr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
      machineState = MACHINE_STATE::IDLE;
      if (Estr->adcCalDone)
        Estr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Estr->adcRange);
      break;

    case MACHINE_STATE::CALIB_RANGES_G:
      Gstr->resetCalibValues(CALIB_TYPE::CALIB_RANGE);
      Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      Gstr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
      machineState = MACHINE_STATE::IDLE;
      if (Gstr->adcCalDone)
        Gstr->saveToEeprom(CALIB_TYPE::CALIB_RANGE, (uint8_t*)&Gstr->adcRange);
      break;

    case MACHINE_STATE::MEASURING:
      break;

    default:
      break;
  }
}

/* 
  When a request is received, always send an acknowledgement of it, followed by either requested values (same packet) or measurement values (different packets continuous)
*/
MACHINE_STATE parseRequests(HID_REQUESTS* req)
{
  bool msgFormat = false;
  uint8_t len, pos;
  MACHINE_STATE retVal = MACHINE_STATE::ERROR; //machineState;

  len = (uint8_t)req[1];
  pos = (uint8_t)req[1] + 1;
  if ((req[0] == HID_REQUESTS::COMMAND) &&
      (req[pos] == HID_REQUESTS::END) &&
      (len > 0))
  {
    msgFormat = true;
  }

  /* Request format is matching the expectations: we can parse the message */
  if (msgFormat)
  {
    uint8_t notifLen  = 0;
    uint8_t notif[64] = {0};
    notif[0]          = (uint8_t)HID_NOTIFICATIONS::ACKNOWLEDGEMENT;

    switch ((HID_REQUESTS)req[2])
    {
      case HID_REQUESTS::MEASURE:
        notif[1]            = 3;
        notif[2]            = (uint8_t)HID_NOTIFICATIONS::MEASURE_REQ;
        notif[3]            = (uint8_t)machineState;
        notif[notif[1] + 1] = (uint8_t)HID_NOTIFICATIONS::END;
        retVal              = MACHINE_STATE::MEASURING;
        break;

      case HID_REQUESTS::CALIB_RANGES:
        notif[1]            = 4;
        notif[2]            = (uint8_t)HID_NOTIFICATIONS::CALIB_RANGES;
        notif[4]            = (uint8_t)machineState;
        notif[notif[1] + 1] = (uint8_t)HID_NOTIFICATIONS::END;
        if (req[3] == HID_REQUESTS::STRING_G)
        {
          notif[3] = (uint8_t)HID_NOTIFICATIONS::STRING_G;
          retVal   = MACHINE_STATE::CALIB_RANGES_G;
        }
        else if (req[3] == HID_REQUESTS::STRING_E)
        {
          notif[3] = (uint8_t)HID_NOTIFICATIONS::STRING_E;
          retVal   = MACHINE_STATE::CALIB_RANGES_E;
        }
        else
        {
          notif[1] = 3;
          notif[2] = (uint8_t)HID_NOTIFICATIONS::ERROR_BADCMD;
          notif[3] = (uint8_t)machineState;
          notif[4] = (uint8_t)HID_NOTIFICATIONS::END;
        }
        break;

      case HID_REQUESTS::CALIB_TOUCH:
        notif[1] = 4;
        notif[2] = (uint8_t)HID_NOTIFICATIONS::CALIB_TOUCH;
        notif[4] = (uint8_t)machineState;
        notif[5] = (uint8_t)HID_NOTIFICATIONS::END;
        if (req[3] == HID_REQUESTS::STRING_G)
        {
          notif[3] = (uint8_t)HID_NOTIFICATIONS::STRING_G;
          retVal   = MACHINE_STATE::CALIB_TOUCH_G;
        }
        else if (req[3] == HID_REQUESTS::STRING_E)
        {
          notif[3] = (uint8_t)HID_NOTIFICATIONS::STRING_E;
          retVal   = MACHINE_STATE::CALIB_TOUCH_E;
        }
        else
        {
          notif[1] = 3;
          notif[2] = (uint8_t)HID_NOTIFICATIONS::ERROR_BADCMD;
          notif[3] = (uint8_t)machineState;
          notif[4] = (uint8_t)HID_NOTIFICATIONS::END;
        }
        break;

      case HID_REQUESTS::VIEW:
        break;

      case HID_REQUESTS::EXIT:
        break;

      default:
        notif[1] = 3;
        notif[2] = (uint8_t)HID_NOTIFICATIONS::ERROR_BADCMD;
        notif[3] = (uint8_t)machineState;
        notif[4] = (uint8_t)HID_NOTIFICATIONS::END;
        break;
    }

    RawHID.send(notif, 50);
  }

  return retVal;
}

//   case HID_REQUESTS::STRING_E:
//   case HID_REQUESTS::STRING_G:
//     if (machineState == MACHINE_STATE::CALIB_RANGES)
//     {
//       send[1] = (byte)cmd;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//       if (cmd == HID_REQUESTS::STRING_E)
//         Estr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
//       else
//         Gstr->calibrate(CALIB_TYPE::CALIB_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
//     }
//     else if (machineState == MACHINE_STATE::CALIB_TOUCH)
//     {
//       send[1] = (byte)cmd;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//       if (cmd == HID_REQUESTS::STRING_E)
//       {
//         Estr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
//         Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
//         Estr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh);
//         if (Estr->touchCalDone) Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
//       }
//       else
//       {
//         Gstr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
//         Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
//         Gstr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh);
//         if (Gstr->touchCalDone) Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
//       }
//     }
//     else
//     {
//       send[1] = (byte)HID_REQUESTS::ERR_NOCMD;
//       send[2] = (byte)machineState;
//       RawHID.send(send, 64);
//     }
//     break;

//   case HID_REQUESTS::CALIB_RANGES:
//   case HID_REQUESTS::CALIB_TOUCH:
//     if (machineState == MACHINE_STATE::IDLE)
//     {
//       send[1] = (byte)cmd;
//       if (cmd == HID_REQUESTS::CALIB_RANGES)
//       {
//         machineState = MACHINE_STATE::CALIB_RANGES;
//         send[2]      = (byte)machineState;
//         RawHID.send(send, 64);
//       }
//       if (cmd == HID_REQUESTS::CALIB_TOUCH)
//       {
//         machineState = MACHINE_STATE::CALIB_TOUCH;
//         send[2]      = (byte)machineState;
//         RawHID.send(send, 64);
//         // calibrateTouch();
//       }
//     }
//     else
//     {
//       send[1]      = (byte)HID_REQUESTS::ERR_NOCMD;
//       machineState = MACHINE_STATE::IDLE;
//       send[2]      = (byte)machineState;
//       RawHID.send(send, 64);
//     }

//     break;

//   case HID_REQUESTS::MEASURE:
//     if (machineState == MACHINE_STATE::IDLE)
//     {
//       send[1]      = (byte)cmd;
//       machineState = MACHINE_STATE::MEASURING;
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

//   case HID_REQUESTS::HELP:
//     if ((machineState == MACHINE_STATE::IDLE) || (machineState == MACHINE_STATE::CALIB_RANGES) || (machineState == MACHINE_STATE::CALIB_TOUCH))
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

//   case HID_REQUESTS::VIEW:
//     if ((machineState == MACHINE_STATE::IDLE) || (machineState == MACHINE_STATE::CALIB_RANGES) || (machineState == MACHINE_STATE::CALIB_TOUCH))
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

//   case HID_REQUESTS::EXIT:
//     send[1]      = (byte)cmd;
//     machineState = MACHINE_STATE::IDLE;
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
// while (machineState == MACHINE_STATE::CALIB_TOUCH)
// {
//   str->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &str->adcRange, &str->touchThresh);
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
//     Estr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
//     Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);
//     if (Estr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh))
//       Serial.print("Calibration OK! ");
//     else
//       Serial.print("Calibration poor! ");

//     Serial.printf("Thresholds on E string: 0x%04x - 0x%04x (avg: 0x%04x)\n", Estr->touchThresh.min, Estr->touchThresh.max, Estr->touchThresh.avg);
//     if (Estr->touchCalDone) Estr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Estr->touchThresh);

//     displayHelp();
//   }
//   else if (c == 'g')
//   {
//     Serial.printf("Calibrating G string...\nTouch the string at different places on different manners ('x' to finish (t: %d, a:%d))\n", Gstr->touchPin, Gstr->adcPin);
//     Gstr->resetCalibValues(CALIB_TYPE::CALIB_TOUCH);
//     Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);
//     if (Gstr->calibrate(CALIB_TYPE::CALIB_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh))
//       Serial.print("Calibration OK! ");
//     else
//       Serial.print("Calibration poor! ");

//     Serial.printf("Thresholds on G string: 0x%04x - 0x%04x (avg: 0x%04x)\n", Gstr->touchThresh.min, Gstr->touchThresh.max, Gstr->touchThresh.avg);
//     if (Gstr->touchCalDone) Gstr->saveToEeprom(CALIB_TYPE::CALIB_TOUCH, (uint8_t*)&Gstr->touchThresh);

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
//     machineState = MACHINE_STATE::IDLE;
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
  while (machineState == MACHINE_STATE::MEASURING)
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
        machineState = MACHINE_STATE::IDLE;
        displayHelp();
      }
    }
  }

  //   char c = 0;

  //   while (machineState == MACHINE_STATE::MEASURING)
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
  if (machineState == MACHINE_STATE::IDLE)
  {
    Serial.print("'r' : calibrate string pressure RANGE\n't' : calibrate string TOUCH thresholds\n"
                 "'m' : MEASURE string pressures (continuous)\n'v' : VIEW current calibration values\n"
                 "'h' : display HELP\n'x' : EXIT calibration or measuring modes\n");
  }
  else if ((machineState == MACHINE_STATE::CALIB_RANGES_G) || (machineState == MACHINE_STATE::CALIB_TOUCH_G))
  {
    Serial.printf("'g': G string\n'e': E string\n'v': VIEW current calibrations\n'x': EXIT.\n");
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
