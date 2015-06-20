#include "Log.h"


uint8_t _warn[MAX_WARNINGS];
uint8_t _error[MAX_ERRORS];

uint8_t warnCount = 0, errorCount = 0;
boolean warningOverflow = false, errorOverflow = false;

void warn(uint8_t code){
  warnCount++;
  if (warnCount >= MAX_WARNINGS){ 
    warningOverflow = true;
    return;
  }
  
  _warn[warnCount - 1] = code;
}

void error(uint8_t code){
  errorCount++;
  
  if (errorCount >= MAX_ERRORS){ 
    errorOverflow = true;
    return;
  }
  
  _error[errorCount - 1] = code;
}


void transmitWarnings(){
  
  if (warningOverflow == true){
    Serial.println("error"); 
    Serial.println(255);
    Serial.println("end");
    warningOverflow = false;
  }
  
  if (warnCount > 0){
    Serial.println("warn");
    while (warnCount > 0){
      Serial.println(_warn[warnCount - 1]); 
      warnCount--;
    }
    Serial.println("end");
  }
}

void transmitErrors(){
  
  if (errorOverflow == true){
    Serial.println("error"); 
    Serial.println(255);
    Serial.println("end");
    warningOverflow = false;
  }
  
  if (errorCount > 0){
	Serial.println("error");
	while (errorCount > 0){
	Serial.println(_error[errorCount - 1]); 
	  errorCount--;
	}
	Serial.println("end");
  }
}


void transmitLogs(){
  transmitWarnings();
  transmitErrors();
}


