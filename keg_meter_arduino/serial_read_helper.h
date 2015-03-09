
#ifndef SERIAL_READ_HELPER_H_
#define SERIAL_READ_HELPER_H_

#include <Arduino.h>

void convertBytesToT(float& f, const char* bytes) {
  f = atof(bytes); 
}

void convertBytesToT(int& i, const char* bytes) {
  i = atoi(bytes);
}

template <class T> T readTFromSerialUntil(char untilChar) {
  int count = 0;
  #define NUM_BUFFER_BYTES 16
  char bytes[NUM_BUFFER_BYTES] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
  char serialReadByte = ' ';
  
  while (true && count < NUM_BUFFER_BYTES) {
    while (Serial.available() == 0);
    serialReadByte = Serial.read();
    if (serialReadByte == untilChar) {
      break;
    }
    
    bytes[count++] = serialReadByte;
  }
  
  if (count >= NUM_BUFFER_BYTES) {
    Serial.println("Buffer overflowed while reading data.");
    return static_cast<T>(0);
  }
  
  T returnVal;
  convertBytesToT(returnVal, bytes);
  return returnVal;
}

#endif // SERIAL_READ_HELPER_H_
