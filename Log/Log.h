#ifndef LOG_H
#define LOG_H
  
  #include "Arduino.h"
  #define MAX_WARNINGS   20
  #define MAX_ERRORS     10
  
  extern uint8_t _warn[MAX_WARNINGS];
  extern uint8_t _error[MAX_ERRORS];

  void warn(uint8_t code);
  void error(uint8_t code);
  void transmitWarnings();
  void transmitErrors();
  void transmitLogs();
  
#endif
