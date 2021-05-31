#include "Arduino.h"
#include <ADC.h>
#include <ADC_util.h>

void calibrate();
void doCalibrate(uint8_t strNb);
void measure();
void measureInterrupts();
void measurePDB();
uint16_t measureString(uint8_t strNb);
void displayString(uint16_t strVal);
void displayHelp();

const int strPins[] = {A0, A1};
elapsedMicros elapsedTime;

ADC* adc = new ADC(); // adc object;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode((uint8_t)strPins[0], INPUT);
  pinMode((uint8_t)strPins[1], INPUT);

  Serial.begin(115200);
  delay(1000);

  adc->adc0->setAveraging(8);                                          // set number of averages
  adc->adc0->setResolution(10);                                        // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);     // change the sampling speed

  displayHelp();
}

uint16_t strValues[2];
uint8_t displayvalues[2];
uint16_t strRanges[2][2] = {{UINT16_MAX, 0}, {UINT16_MAX, 0}};

void loop()
{
  char c = 0;
  if (Serial.available())
  {
    c = Serial.read();
    if (c == 'c')
    { // calibration
      calibrate();
    }
    else if (c == 'i')
    {
      measureInterrupts();
    }
    else if (c == 'm')
    {
      measure();
    }
    else if (c == 'p')
    {
      measurePDB();
    }
    else if (c == 'h')
    {
      displayHelp();
    }
  }

  //    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));
}

void calibrate()
{
  char c           = 0;
  bool calibrating = true;

  Serial.println("Calibrate pressure ranges.");
  while (calibrating)
  {
    Serial.print("Press 'g' for G string, 'e' for E string or 'x' to exit: ");
    while (!Serial.available())
      ;
    c = Serial.read();
    if (c == 'g')
    {
      Serial.println("Calibrating G string, press 'x' to finish");
      doCalibrate(0);
      displayHelp();
    }
    else if (c == 'e')
    {
      Serial.println("Calibrating E string, press 'x' to finish");
      doCalibrate(1);
      displayHelp();
    }
    else if (c == 'x')
    {
      Serial.printf("\nRanges: G -> %d-%d; E -> %d-%d\n", strRanges[0][0], strRanges[0][1], strRanges[1][0], strRanges[1][1]);
      Serial.println("Exit calibration");
      calibrating = false;
    }
    else
    {
      Serial.println("Please choose a valid option");
    }
  }
}

void doCalibrate(uint8_t strNb)
{
  uint16_t value;
  bool calibrating = true;
  uint32_t timeCnt = 0;
  uint16_t range[] = {UINT16_MAX, 0};
  char c           = 0;

  while (calibrating)
  {
    value = measureString(strNb);
    displayString(value);

    if (value < range[0])
    {
      range[0] = value;
      timeCnt  = 0;
    }
    else if (value > range[1])
    {
      range[1] = value;
      timeCnt  = 0;
    }
    else
    {
      timeCnt++;
    }

    if (Serial.available())
    {
      c = Serial.read();
      if (c == 'x')
      {
        switch (strNb)
        {
          case 0:
            strRanges[0][0] = range[0];
            strRanges[0][1] = range[1];
            break;
          case 1:
            strRanges[1][0] = range[0];
            strRanges[1][1] = range[1];
            break;
          default:
            break;
        }
        Serial.println("Exiting calibration");
        Serial.print("Range: ");
        Serial.print(range[0]);
        Serial.print(" - ");
        Serial.println(range[1]);
        calibrating = false;
      }
    }
  }
}

void measure()
{
  char c         = 0;
  bool measuring = true;

  while (measuring)
  {
    // Single reads
    strValues[0] = measureString(0);
    strValues[1] = measureString(1);

    Serial.print("G: ");
    displayString(strValues[0]);

    Serial.print("E: ");
    displayString(strValues[1]);

    Serial.println("");
    // Print errors, if any.
    if (adc->adc0->fail_flag != ADC_ERROR::CLEAR)
    {
      Serial.print("ADC0: ");
      Serial.println(getStringADCError(adc->adc0->fail_flag));
    }

    if (Serial.available())
    {
      c = Serial.read();
      if (c == 'x')
      {
        displayHelp();
        measuring = false;
      }
    }
  }
}

void measureInterrupts()
{
  elapsedTime = 0;
  bool retVal = false;
  adc->adc0->enableInterrupts(adc0_isr);
  if (!(retVal = adc->adc0->startSingleRead(strPins[0])))
  {
    Serial.println("Error on starting single read");
  }

  displayHelp();
}

void measurePDB()
{
  elapsedTime = 0;
  adc->adc0->stopPDB();
  adc->adc0->startSingleRead(strPins[0]);
  adc->adc0->enableInterrupts(adc0_isr);
  adc->adc0->startPDB(1000);
}

uint16_t measureString(uint8_t strNb)
{
  return adc->adc0->analogRead(strPins[strNb]);
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
  Serial.println("'c' : calibrate string pressure ranges");
  Serial.println("'i' : measure string G (interrupt)");
  Serial.println("'m' : measure string pressures (continuous)");
  Serial.println("'p' : measure with PDB timer");
  Serial.println("'h' : display help");
  Serial.println("'x' : exit calibration or measuring mode");
}

// If you enable interrupts make sure to call readSingle() to clear the interrupt.
void adc0_isr()
{
  Serial.println(elapsedTime);
  adc->adc0->readSingle();
}