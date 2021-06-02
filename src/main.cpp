#include "main.h"
#include "Arduino.h"
#include "vString.h"
#include <ADC.h>
#include <ADC_util.h>

elapsedMicros elapsedTime;
ADC* adc      = new ADC(); // adc object;
vString* Gstr = new vString(A0, 'G', 0);
vString* Dstr = new vString(A2, 'D', 1);
vString* Astr = new vString(A3, 'A', 2);
vString* Estr = new vString(A1, 'E', 3);

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

  // Serial.println("G string settings:");
  // Serial.printf("Pin > %d; name > %c; range > %d - %d\n", Gstr->adcPin, Gstr->strName, Gstr->calibRange.min, Gstr->calibRange.max);
  // Serial.println("");
  // Serial.println("E string settings:");
  // Serial.printf("Pin > %d; name > %c; range > %d - %d\n", Estr->adcPin, Estr->strName, Estr->calibRange.min, Estr->calibRange.max);
  displayHelp();
}

void loop()
{
  char c = 0;
  if (Serial.available())
  {
    c = Serial.read();
    if (c == 'c')
    {
      machineState = MACHINE_STATE::CALIBRATING;
      calibrate();
    }
    else if (c == 'm')
    {
      if (checkCalib())
      {
        machineState = MACHINE_STATE::MEASURING;
        measure();
      }
      else
      {
        Serial.println("Please calibrate the string first!");
      }
    }
    else if (c == 'h')
    {
      machineState = MACHINE_STATE::IDLE;
      displayHelp();
    }
  }
}

void calibrate()
{
  char c      = 0;
  bool retVal = false;

  Serial.println("Calibrate pressure ranges.");
  displayHelp();
  while (machineState == MACHINE_STATE::CALIBRATING)
  {
    while (!Serial.available())
      ;

    c = Serial.read();
    if (c == 'a')
    {
      Serial.println("A string not available yet!");
      displayHelp();
    }
    else if (c == 'd')
    {
      Serial.println("D string not available yet!");
      displayHelp();
    }
    else if (c == 'e')
    {
      Serial.println("Calibrating E string, press 'x' to finish");
      Estr->calibOK = false;
      if (retVal = doCalibrate(Estr->adcPin, &Estr->calRange))
      {
        Estr->calibOK = true;
        Serial.print("Calibration OK. ");
      }
      else
        Serial.print("Calibration poor. ");

      Serial.printf("Values on E string: %d - %d\n", Estr->calRange.min, Estr->calRange.max);
      displayHelp();
    }
    else if (c == 'g')
    {
      Serial.println("Calibrating G string, press 'x' to finish");
      Gstr->calibOK = false;
      if (retVal = doCalibrate(Gstr->adcPin, &Gstr->calRange))
      {
        Gstr->calibOK = true;
        Serial.print("Calibration OK. ");
      }
      else
        Serial.print("Calibration poor. ");

      Serial.printf("Vgalues on G string: %d - %d\n", Gstr->calRange.min, Gstr->calRange.max);

      displayHelp();
    }
    else if (c == 'v')
    {
      Serial.println("Current calibration values");
      Serial.printf("G: %d - %d\n", Gstr->calRange.min, Gstr->calRange.max);
      Serial.printf("A: not available yet\n");
      Serial.printf("D: not available yet\n");
      Serial.printf("E: %d - %d\n", Estr->calRange.min, Estr->calRange.max);
      displayHelp();
    }
    else if (c == 'x')
    {
      Serial.println("\nExiting calibration. Ranges:");
      Serial.printf("G: %d - %d\nA: not available yet\nD: not available yet\nE: %d - %d\n\n", Gstr->calRange.min, Gstr->calRange.max, Estr->calRange.min, Estr->calRange.max);
      machineState = MACHINE_STATE::IDLE;
      displayHelp();
    }
    else
    {
      Serial.println("Please choose a valid option");
    }
  }
}

bool doCalibrate(uint8_t pin, minmax_t* range)
{
  bool doCal = true;
  range->min = UINT16_MAX;
  range->max = 0;
  while (doCal)
  {
    uint16_t val = adc->adc0->analogRead(pin);
    // Serial.print("val: ");
    // Serial.println(val);
    if (val > range->max) range->max = val;
    if (val < range->min) range->min = val;
    displayRange(*range);

    if (Serial.available())
    {
      char c = Serial.read();
      if (c == 'x')
      {
        doCal = false;
        if ((range->min < STR_CALIB_MIN) && (range->max > STR_CALIB_MAX))
          return true;
        else
          return false;
      }
    }
  }
  // Serial.printf("Calibration values on current string: %d - %d\n", range->min, range->max);
}

void measure()
{
  bool retVal     = false;
  uint16_t adcVal = 0;
  Gstr->newVal    = false;
  Estr->newVal    = false;

  adc->adc0->enableInterrupts(adc0_isr);
  retVal = adc->adc0->startSingleRead(Gstr->adcPin);
  while (machineState == MACHINE_STATE::MEASURING)
  {
    if (Gstr->newVal)
    {
      if ((Gstr->adcTail + 1) < Gstr->bufferSize)
        Gstr->adcTail++;
      else
        Gstr->adcTail = 0;

      adcVal       = Gstr->adcBuffer[Gstr->adcTail];
      Gstr->newVal = false;
      Serial.printf("G: %d    ", adcVal);
      retVal = adc->adc0->startSingleRead(Estr->adcPin);
    }
    if (Estr->newVal)
    {
      if ((Estr->adcTail + 1) < Estr->bufferSize)
        Estr->adcTail++;
      else
        Estr->adcTail = 0;

      adcVal       = Estr->adcBuffer[Estr->adcTail];
      Estr->newVal = false;
      Serial.printf("E: %d\n", adcVal);
      retVal = adc->adc0->startSingleRead(Gstr->adcPin);
    }
    if (Serial.available())
    {
      char c = Serial.read();
      if (c == 'x')
      {
        displayHelp();
        machineState = MACHINE_STATE::IDLE;
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

void measureString(uint8_t pin)
{
  bool retVal = false;
  // adc->adc0->enableInterrupts(adc0_isr);
  if (!(retVal = adc->adc0->startSingleRead(pin)))
  {
    Serial.printf("Error on startSingleRead(%d)\n", pin);
  }

  displayHelp();
}

void displayRange(minmax_t range)
{
  minmax_t localRange = {.min = UINT16_MAX, .max = 0};
  uint8_t dispMin, dispMax;

  if (range.min < localRange.min)
  {
    localRange.min = range.min;
    dispMin        = 100 * localRange.min / 1023;
    // Serial.printf("New min: %d\n", localRange.min);
  }
  if (range.max > localRange.max)
  {
    localRange.max = range.max;
    dispMax        = 100 * localRange.max / 1023;
    // Serial.printf("New max: %d\n", localRange.max);
  }
  Serial.printf("\r[");
  for (int i = 0; i < 100; i++)
  {
    if (i < dispMin)
      Serial.printf(" ");
    else if ((i > dispMin) && (i < dispMax))
      Serial.printf("*");
    else if (i > dispMax)
      Serial.printf(" ");
  }
  Serial.printf("]\n");
}

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
    Serial.print("'c' : calibrate string pressure ranges\n'm' : measure string pressures (continuous)\n'h' : display help\n'x' : exit calibration or measuring mode\n");
  }
  else if (machineState == MACHINE_STATE::CALIBRATING)
  {
    Serial.printf("'g': G string\n'd': D string\n'd': D string\n'e': E string\n'v': view current calibration\n'x': exit.\n");
  }
}

// If you enable interrupts make sure to call readSingle() to clear the interrupt.
void adc0_isr()
{
  uint8_t pin = ADC::sc1a2channelADC0[ADC0_SC1A & ADC_SC1A_CHANNELS];
  if (pin == Gstr->adcPin)
  {
    if ((Gstr->adcHead + 1) < Gstr->bufferSize)
      Gstr->adcHead++;
    else
      Gstr->adcHead = 0;

    if (Gstr->adcHead != Gstr->adcTail)
    {
      Gstr->adcBuffer[Gstr->adcHead] = adc->adc0->readSingle();
      Gstr->newVal                   = true;
    }
    else
    {
      Serial.println("Buffer overflow on string G");
    }
  }
  else if (pin == Estr->adcPin)
  {
    if ((Estr->adcHead + 1) < Estr->bufferSize)
      Estr->adcHead++;
    else
      Estr->adcHead = 0;

    if (Estr->adcHead != Estr->adcTail)
    {
      Estr->adcBuffer[Estr->adcHead] = adc->adc0->readSingle();
      Estr->newVal                   = true;
    }
    else
    {
      Serial.println("Buffer overflow on string E");
    }
  }
  else
  {
    adc->readSingle();
  }

  if (adc->adc0->adcWasInUse)
  {
    // Serial.println("In use");
    adc->adc0->loadConfig(&adc->adc0->adc_config);
    adc->adc0->adcWasInUse = false;
  }
}

bool checkCalib()
{
  if (Gstr->checkCal() && Estr->checkCal())
    return true;
  else
    return false;
}