#ifndef VSTRING_H
#define VSTRING_H

// #include "main.h"
#include <ADC.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <usb_rawhid.h>

#define BUFFER_SIZE UINT8_MAX
#define EEPROM_RANGE_ADDR 0
#define EEPROM_TOUCH_ADDR 8
#define EEPROM_ADDR_OFFSET 20
#define ADC_RANGE_MIN 200
#define ADC_RANGE_MAX 1000
#define TOUCH_THRESH_MIN 15000
#define TOUCH_THRESH_MAX 30000

typedef struct
{
  uint16_t min, max;
} range_t;

typedef struct
{
  uint16_t min, max, avg;
} thresh_t;

enum class CALIB_TYPE : uint8_t {
  CALIB_NONE = 0,
  CALIB_RANGE,
  CALIB_TOUCH
};

class vString
{
  public:
  char name;
  uint8_t number;
  bool adcNewVal;
  const uint16_t bufSize = BUFFER_SIZE;

  uint8_t adcPin;
  range_t adcRange;
  bool adcCalDone;
  // uint16_t adcBuf[BUFFER_SIZE];
  // uint16_t bufHead, bufTail;
  // uint16_t adcCurVal;
  // uint16_t adcDispValue;

  uint8_t touchPin;
  uint16_t touchBuf[BUFFER_SIZE];
  thresh_t touchThresh;
  bool touchCalDone;

  vString(uint8_t touchPin, uint8_t adcPin, char strName, uint8_t strNumber);
  ~vString();

  void getCalibValues();
  void resetCalibValues(CALIB_TYPE type);

  bool checkCalStatus(CALIB_TYPE type);
  void saveToEeprom(CALIB_TYPE type, uint8_t* data);
  void getFromEeprom(CALIB_TYPE type);
  bool calibrate(CALIB_TYPE type, ADC_Module* module, range_t* range, thresh_t* thresh);
  uint16_t measure(ADC_Module* module, uint8_t smoothing);
  void displayRange(range_t* range);
  void displayTouch(thresh_t* thresh);
  void viewCalibValues();
  void viewStringValues();
};

#endif /* VSTRING_H */