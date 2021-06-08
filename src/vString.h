#ifndef VSTRING_H
#define VSTRING_H

#include <ADC.h>
#include <Arduino.h>
#include <EEPROM.h>

#define BUFFER_SIZE UINT8_MAX
#define EEPROM_RANGE_ADDR 0
#define EEPROM_TOUCH_ADDR 8
#define EEPROM_ADDR_OFFSET 20
#define ADC_RANGE_MIN 200
#define ADC_RANGE_MAX 1000
#define TOUCH_THRESH_MIN 20000
#define TOUCH_THRESH_MAX 22000

typedef struct
{
  uint16_t min, max;
} range_t;

typedef struct
{
  uint16_t min, max, avg;
} thresh_t;

enum strDataType {
  NONE = 0,
  MEAS_RANGE,
  MEAS_TOUCH,
  CAL_RANGE,
  CAL_TOUCH,
  ERROR
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

  bool checkCalStatus(strDataType type);
  void saveToEeprom(strDataType type, uint8_t* data);
  void getFromEeprom(strDataType type, uint8_t* data);
  bool calibrate(strDataType type, ADC_Module* module, range_t* range, thresh_t* thresh);
  void displayRange(range_t* range);
  void displayTouch(thresh_t* thresh);
  void viewCalibValues();
};

#endif /* VSTRING_H */