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

MACHINE_STATE machineState = MACHINE_STATE::IDLE;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(1000);

  adc->adc0->setAveraging(8);                                          // set number of averages
  adc->adc0->setResolution(10);                                        // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);     // change the sampling speed

  Serial.println("G string settings:");
  Serial.printf("touch > %d; ADC > %d; name > %c; range > %d - %d\n", Gstr->touchPin, Gstr->adcPin, Gstr->name, Gstr->adcRange.min, Gstr->adcRange.max);
  Serial.println("");
  Serial.println("E string settings:");
  Serial.printf("touch > %d; ADC > %d; name > %c; range > %d - %d\n", Estr->touchPin, Estr->adcPin, Estr->name, Estr->adcRange.min, Estr->adcRange.max);
  Serial.println("");

  displayHelp();
}

void loop()
{
  char c = 0;
  if (Serial.available())
  {
    c = Serial.read();
    if (c == 'r')
    {
      machineState = MACHINE_STATE::CALIBRATING_RANGE;
      calibrateRange();
    }
    else if (c == 't')
    {
      machineState = MACHINE_STATE::CALIBRATING_TOUCH;
      calibrateTouch();
    }
    else if (c == 'm')
    {
      // if (checkCalib())
      // {
      machineState = MACHINE_STATE::MEASURING;
      measure();
      // }
      // else
      // {
      //   Serial.println("Please calibrate the string first!");
      // }
    }
    else if (c == 'h')
    {
      machineState = MACHINE_STATE::IDLE;
      displayHelp();
    }
  }
}

void calibrateRange()
{
  char c      = 0;
  bool retVal = false;

  Serial.println("Calibrate pressure ranges.");
  displayHelp();
  while (machineState == MACHINE_STATE::CALIBRATING_RANGE)
  {
    while (!Serial.available())
      ;

    c = Serial.read();
    if (c == 'a')
    {
      Serial.println("A string not available yet!");
      // displayHelp();
    }
    else if (c == 'd')
    {
      Serial.println("D string not available yet!");
      // displayHelp();
    }
    else if (c == 'e')
    {
      Serial.printf("Calibrating E string, press 'x' to finish (t: %d, a:%d)\n", Estr->touchPin, Estr->adcPin);
      Estr->adcCalDone = false;
      if (Estr->calibrate(strDataType::CAL_RANGE, adc->adc0, &Estr->adcRange, &Estr->touchThresh))
        Serial.print("Calibration OK! ");
      else
        Serial.print("Calibration poor! ");

      Serial.printf("Values on E string: 0x%04x - 0x%04x\n", Estr->adcRange.min, Estr->adcRange.max);
      if (Estr->adcCalDone) Estr->saveToEeprom(strDataType::CAL_RANGE, (uint8_t*)&Estr->adcRange);

      displayHelp();
    }
    else if (c == 'g')
    {
      Serial.printf("Calibrating G string, press 'x' to finish (t: %d, a:%d)\n", Gstr->touchPin, Gstr->adcPin);
      Gstr->adcCalDone = false;
      if (Gstr->calibrate(strDataType::CAL_RANGE, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh))
        Serial.print("Calibration OK! ");
      else
        Serial.print("Calibration poor! ");

      Serial.printf("Values on G string: 0x%04x - 0x%04x\n", Gstr->adcRange.min, Gstr->adcRange.max);
      if (Gstr->adcCalDone) Gstr->saveToEeprom(strDataType::CAL_RANGE, (uint8_t*)&Gstr->adcRange);
      displayHelp();
    }
    else if (c == 'v')
    {
      Serial.println("Current calibration values");
      Gstr->viewCalibValues();
      Estr->viewCalibValues();
      displayHelp();
    }
    else if (c == 'x')
    {
      Serial.println("\nExiting calibration. Ranges:");
      Serial.printf("G: %d - %d\nA: not available yet\nD: not available yet\nE: %d - %d\n\n", Gstr->adcRange.min, Gstr->adcRange.max, Estr->adcRange.min, Estr->adcRange.max);
      machineState = MACHINE_STATE::IDLE;
      displayHelp();
    }
    else
    {
      Serial.println("Please choose a valid option");
    }
  }
}

void calibrateTouch()
{
  char c      = 0;
  bool retVal = false;

  Serial.println("Calibrate touch thresholds.");
  displayHelp();
  while (machineState == MACHINE_STATE::CALIBRATING_TOUCH)
  {
    while (!Serial.available())
      ;

    c = Serial.read();
    if (c == 'a')
    {
      Serial.println("A string not available yet!");
    }
    else if (c == 'd')
    {
      Serial.println("D string not available yet!");
    }
    else if (c == 'e')
    {
      Serial.printf("Calibrating E string...\nTouch the string at different places on different manners ('x' to finish (t: %d, a:%d))\n", Estr->touchPin, Estr->adcPin);
      Estr->touchCalDone = false;
      if (Estr->calibrate(strDataType::CAL_TOUCH, adc->adc0, &Estr->adcRange, &Estr->touchThresh))
        Serial.print("Calibration OK! ");
      else
        Serial.print("Calibration poor! ");

      Serial.printf("Thresholds on E string: %d - %d (avg: %d)\n", Estr->touchThresh.min, Estr->touchThresh.max, Estr->touchThresh.avg);
      // if (Estr->touchCalDone) Estr->saveToEeprom(strDataType::CAL_TOUCH, (uint8_t*)&Estr->touchThresh);

      displayHelp();
    }
    else if (c == 'g')
    {
      Serial.printf("Calibrating G string...\nTouch the string at different places on different manners ('x' to finish (t: %d, a:%d))\n", Gstr->touchPin, Gstr->adcPin);
      Gstr->touchCalDone = false;
      if (Gstr->calibrate(strDataType::CAL_TOUCH, adc->adc0, &Gstr->adcRange, &Gstr->touchThresh))
        Serial.print("Calibration OK! ");
      else
        Serial.print("Calibration poor! ");

      Serial.printf("Thresholds on G string: %d -%d (avg: %d)\n", Gstr->touchThresh.min, Gstr->touchThresh.max, Gstr->touchThresh.avg);
      // if (Gstr->touchCalDone) Gstr->saveToEeprom(strDataType::CAL_TOUCH, (uint8_t*)&Estr->touchThresh);

      displayHelp();
    }
    else if (c == 'v')
    {
      Serial.println("Current calibration values");
      Gstr->viewCalibValues();
      Estr->viewCalibValues();
      displayHelp();
    }
    else if (c == 'x')
    {
      Serial.println("\nExiting calibration. Values:");
      Serial.printf("G: adc -> %d - %d     touch -> %d\n", Gstr->adcRange.min, Gstr->adcRange.max, Gstr->touchThresh.avg);
      Serial.printf("E: adc -> %d - %d     touch -> %d\n", Estr->adcRange.min, Estr->adcRange.max, Estr->touchThresh.avg);
      machineState = MACHINE_STATE::IDLE;
      displayHelp();
    }
    else
    {
      Serial.println("Please choose a valid option");
    }
  }
}

void measure()
{
  uint16_t adcVal   = 0;
  uint16_t touchVal = 0;
  uint32_t eStart   = millis();
  uint32_t gStart   = millis();
  Gstr->adcNewVal   = false;
  Estr->adcNewVal   = false;

  // adc->adc0->enableInterrupts(adc0_isr);
  while (machineState == MACHINE_STATE::MEASURING)
  {
    if ((touchVal = touchRead(Gstr->touchPin)) > 20000)
    {
      adcVal = adc->adc0->analogRead(Gstr->adcPin);

      // if ((Gstr->adcTail + 1) < Gstr->bufferSize)
      //   Gstr->adcTail++;
      // else
      //   Gstr->adcTail = 0;

      // adcVal = Gstr->adcBuffer[Gstr->adcTail];
      if ((millis() - gStart) > 100)
      {
        Serial.print(gDetla);
        Serial.printf(" G: %d - %d\n", touchVal, adcVal);
        gDetla = 0;
        gStart = millis();
      }
    }

    if ((touchVal = touchRead(Estr->touchPin)) > 20000)
    {
      adcVal = adc->adc0->analogRead(Estr->adcPin);

      // if ((Estr->adcTail + 1) < Estr->bufferSize)
      //   Estr->adcTail++;
      // else
      //   Estr->adcTail = 0;

      // adcVal       = Estr->adcBuffer[Estr->adcTail];
      if ((millis() - eStart) > 100)
      {
        Serial.print(eDelta);
        Serial.printf(" E: %d - %d\n", touchVal, adcVal);
        eDelta = 0;
        eStart = millis();
      }
    }

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
                 "'m' : MEASURE string pressures (continuous)\n'h' : display help\n'x' : exit calibration or measuring modes\n");
  }
  else if ((machineState == MACHINE_STATE::CALIBRATING_RANGE) || (machineState == MACHINE_STATE::CALIBRATING_TOUCH))
  {
    Serial.printf("'g': G string\n'd': D string\n'd': D string\n'e': E string\n'v': view current calibration\n'x': exit.\n");
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
