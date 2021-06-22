#ifndef _MESSAGES_H
#define _MESSAGES_H

#include <Arduino.h>

enum class HID_REQ : uint8_t {
  // request type headers (< 0x20)
  R_CMD = 0x10,
  // requests values (0x20 <= val <= 0x7E)
  R_STR_A   = 'A',
  R_STR_D   = 'D',
  R_STR_E   = 'E',
  R_STR_G   = 'G',
  R_HELP    = 'h',
  R_MEAS    = 'm',
  R_CALIB_R = 'r',
  R_CALIB_T = 't',
  R_VIEW    = 'v',
  R_EXIT    = 'x',
  // request end delimiter
  R_END = 0xFF,
};

enum class HID_NOTIF : uint8_t {
  // notification type headers
  N_MEAS = 0x00,
  N_ACK  = 0x01,
  N_INFO = 0x02,
  // notification values
  N_CALIB_R = (uint8_t)HID_REQ::R_CALIB_R,
  N_CR_DONE = 0x21,
  N_CALIB_T = (uint8_t)HID_REQ::R_CALIB_T,
  N_CT_DONE = 0x31,
  N_VIEW    = (uint8_t)HID_REQ::R_VIEW,
  N_EXIT    = (uint8_t)HID_REQ::R_EXIT,
  N_STR_E   = (uint8_t)HID_REQ::R_STR_E,
  N_STR_G   = (uint8_t)HID_REQ::R_STR_G,
  N_STR_D   = (uint8_t)HID_REQ::R_STR_D,
  N_STR_A   = (uint8_t)HID_REQ::R_STR_A,
  N_ERR     = 0xE0,
  N_TIMEOUT = 0xE1,
  // notification end delimiter
  N_END = 0xFF
};

#endif /* _MESSAGES_H */