#include "main.h"
#include "Arduino.h"
#include "vString.h"
#include <ADC.h>
#include <ADC_util.h>

elapsedMicros elapsedTime;
ADC* adc      = new ADC();       // adc object;
vString* Gstr = new vString(A0); //, 'G');
vString* Estr = new vString(A1); //, 'E');

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
      machineState = MACHINE_STATE::MEASURING;
      measure();
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
  char c = 0;

  Serial.println("Calibrate pressure ranges.");
  while (machineState == MACHINE_STATE::CALIBRATING)
  {
    Serial.println("Press 'g' for G string, 'e' for E string or 'x' to exit.");
    while (!Serial.available())
      ;

    c = Serial.read();
    if (c == 'g')
    {
      Serial.println("Calibrating G string, press 'x' to finish");
      doCalibrate(Gstr->pin);
      displayHelp();
    }
    else if (c == 'e')
    {
      Serial.println("Calibrating E string, press 'x' to finish");
      doCalibrate(Estr->pin);
      displayHelp();
    }
    else if (c == 'x')
    {
      Serial.println("\nExiting calibration");
      machineState = MACHINE_STATE::IDLE;
    }
    else
    {
      Serial.println("Please choose a valid option");
    }
  }
}

void doCalibrate(uint8_t adcPin)
{
  for (int i = 0; i < 200; i++)
  {
    Serial.print(".");
  }
  Serial.println("");
  // bool doCal = true;
  // while (doCal)
  // {
  // }
  // uint16_t value;
  // bool calibrating = true;
  // uint32_t timeCnt = 0;
  // uint16_t range[] = {UINT16_MAX, 0};
  // char c           = 0;

  // while (calibrating)
  // {
  //   value = measureString(strNb);
  //   displayString(value);

  //   if (value < range[0])
  //   {
  //     range[0] = value;
  //     timeCnt  = 0;
  //   }
  //   else if (value > range[1])
  //   {
  //     range[1] = value;
  //     timeCnt  = 0;
  //   }
  //   else
  //   {
  //     timeCnt++;
  //   }

  //   if (Serial.available())
  //   {
  //     c = Serial.read();
  //     if (c == 'x')
  //     {
  //       switch (strNb)
  //       {
  //         case 0:
  //           strRanges[0][0] = range[0];
  //           strRanges[0][1] = range[1];
  //           break;
  //         case 1:
  //           strRanges[1][0] = range[0];
  //           strRanges[1][1] = range[1];
  //           break;
  //         default:
  //           break;
  //       }
  //       Serial.println("Exiting calibration");
  //       Serial.print("Range: ");
  //       Serial.print(range[0]);
  //       Serial.print(" - ");
  //       Serial.println(range[1]);
  //       calibrating = false;
  //     }
  //   }
  // }
}

void measure()
{
  for (int i = 0; i < 200; i++)
  {
    Serial.print(".");
  }
  Serial.println("");
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

  //     if (Serial.available())
  //     {
  //       c = Serial.read();
  //       if (c == 'x')
  //       {
  //         displayHelp();
  //         measuring = false;
  //       }
  //     }
  //   }
}

void measureString(uint8_t adcPin)
{
  bool retVal = false;
  // adc->adc0->enableInterrupts(adc0_isr);
  if (!(retVal = adc->adc0->startSingleRead(adcPin)))
  {
    Serial.printf("Error on startSingleRead(%d)\n", adcPin);
  }

  displayHelp();
}

// uint16_t measureString(uint8_t pin)
// {
//   return adc->adc0->analogRead(pin);
// }

// void displayString(uint16_t strVal)
// {
//   uint8_t disp = uint8_t(strVal * 99 / adc->adc0->getMaxValue());
//   for (int i = 0; i < disp; i++)
//   {
//     Serial.print("*");
//   }
//   Serial.println("");
// }

void displayHelp()
{
  Serial.println("'c' : calibrate string pressure ranges");
  Serial.println("'m' : measure string pressures (continuous)");
  Serial.println("'h' : display help");
  Serial.println("'x' : exit calibration or measuring mode");
}

// If you enable interrupts make sure to call readSingle() to clear the interrupt.
void adc0_isr()
{
  uint8_t pin = ADC::sc1a2channelADC0[ADC0_SC1A & ADC_SC1A_CHANNELS];
  if (pin == Gstr->pin)
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
  else if (pin == Estr->pin)
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
    Serial.println("In use");
    adc->adc0->loadConfig(&adc->adc0->adc_config);
    adc->adc0->adcWasInUse = false;
  }
}