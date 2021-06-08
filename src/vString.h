#ifndef VSTRING_H
#define VSTRING_H

#include <ADC.h>
#include <Arduino.h>
#include <EEPROM.h>

#define BUFFER_SIZE 500
#define EEPROM_RANGE_ADDR 0
#define EEPROM_TOUCH_ADDR 8
#define EEPROM_ADDR_OFFSET 20
#define STR_CALIB_MIN 200
#define STR_CALIB_MAX 1000

typedef struct
{
  uint16_t min, max;
} minmax_t;

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
  bool newVal;
  bool calibOK;
  const uint16_t bufSize = BUFFER_SIZE;
  uint8_t adcPin;
  minmax_t adcRange;
  uint16_t adcBuf[BUFFER_SIZE];
  uint16_t bufHead, bufTail;
  uint16_t adcCurVal;
  uint16_t adcDispValue;

  uint8_t touchPin;
  minmax_t touchRange;

  vString(uint8_t touchPin, uint8_t adcPin, char strName, uint8_t strNumber);
  ~vString();

  bool checkCal();
  bool stringActive(uint8_t touchPin, uint16_t thresh);
  void saveToEeprom(strDataType type, uint8_t* data);
  void getFromEeprom(strDataType type, uint8_t* data);
  bool calibrate(strDataType type, ADC_Module* module, minmax_t* range, minmax_t* thresh);
  void displayRange(minmax_t* range);
};

#endif /* VSTRING_H */