#include "keg_meter_protocol.h"
#include "keg_load_meter.h"

#define MEASUREMENT_MSG_TYPE_STR "M"

#define METER_IDX_BYTE_WIDTH 2
#define SEPARATOR_CHAR ' '
#define METER_PERCENT_CMD_CHAR 'P'
#define METER_ROUTINE_CMD_CHAR 'R'

#define METER_ROUTINE_OFF_CHAR 'O'
#define METER_ROUTINE_CALBRATING_CHAR 'C'
#define METER_ROUTINE_FILLING_CHAR 'F'
#define METER_ROUTINE_MEASURING_CHAR 'M'
#define METER_ROUTINE_BECAME_EMPTY_CHAR 'E'

#define PKG_BEGIN_CHAR '['

void KegMeterProtocol::PrintMeasurementMsg(uint8_t meterIdx, float measurement) { 
  PrintStartPkg();
  KegMeterProtocol::PrintKegNumberStr(meterIdx);
  Serial.print(METER_ID_SEPARATOR_STR);
  Serial.print(MEASUREMENT_MSG_TYPE_STR);
  Serial.print(METER_ID_SEPARATOR_STR);
  KegMeterProtocol::PrintWithZeroPadding(measurement, 3, 3);
  PrintEndPkg();
}

// Format: [<meterIdx> <cmd_char> <data>]
// <meterIdx> is in the form "00" (e.g., meter of index 1 would be 001)
// <cmd_char> is in the form 'X' (i.e., a single character that describes the type of command)
// <data> depends on the type of message:
// METER_PERCENT_CMD_CHAR: <data> == "0.00" (4 bytes defining a percentage of the meter in [0,1])
// METER_ROUTINE_CMD_CHAR: <data> == 'X' (1 byte that defines the type of routine -- see constants)

void KegMeterProtocol::ReadSerial(KegLoadMeter* kegMeters, int numMeters) {
  
  
  // We need to find the start of the package first...
  while (Serial.available() > 0 && Serial.peek() != PKG_BEGIN_CHAR) { Serial.read(); } 
  if (Serial.available() < 8) { return; }
  
  char tempChar;
  
  #define ON_AVAILABLE_TIMEOUT(x) Serial.print("ERROR "); Serial.println(x); return

  // Read the PKG_BEGIN_CHAR
  tempChar = Serial.read();
  if (tempChar != PKG_BEGIN_CHAR) { Serial.print("FAILED: "); Serial.println((int)tempChar); return; }
  
  // We'd like to be able to read the keg index
  if (!WaitForAvailable(2)) { ON_AVAILABLE_TIMEOUT(1); }
  int meterIdx = Serial.parseInt();
  if (meterIdx < 0 || meterIdx >= numMeters) {
    Serial.println("WARNING: Invalid meter index.");
    return; 
  }
  
  KegLoadMeter& selectedMeter = kegMeters[meterIdx];

  // There should be a single byte separator, the type of command, and another single byte separator
  if (!WaitForAvailable(3)) { ON_AVAILABLE_TIMEOUT(2); }
  
  // Read the separator
  tempChar = Serial.read();
  if (tempChar != SEPARATOR_CHAR) { ON_AVAILABLE_TIMEOUT(3); }
  
  // Read the type of message
  char msgCmdType = Serial.read();
  
  // Read the separator
  tempChar = Serial.read();
  if (tempChar != SEPARATOR_CHAR) { ON_AVAILABLE_TIMEOUT(4); }
 
  switch (msgCmdType) {
    
    case METER_PERCENT_CMD_CHAR: {
      // This command will tell a specific meter what percentage it should be at
      // Format of the data is 0.00 (4 bytes)
      if (!WaitForAvailable(4)) { ON_AVAILABLE_TIMEOUT(5); }
      float percent = Serial.parseFloat();
      
      if (percent < 0) { percent = 0; }
      else if (percent > 1) { percent = 1; }
      
      selectedMeter.setPercentage(percent);
      Serial.print("SUCCESS: Percent set to "); Serial.println(percent);
      break;
    }
    
    case METER_ROUTINE_CMD_CHAR: {
      // This command will tell a specific meter what lighting routine it should be running
      // Format of the data is a routine type character (1 byte)
      if (!WaitForAvailable(1)) { ON_AVAILABLE_TIMEOUT(6); }
      char routineType = Serial.read();
      switch (routineType) {
        
        case METER_ROUTINE_OFF_CHAR:
          selectedMeter.setOffRoutine();
          break;
          
        case METER_ROUTINE_CALBRATING_CHAR:
          selectedMeter.setCalibratingRoutine();
          break;
          
        case METER_ROUTINE_FILLING_CHAR:
          selectedMeter.setFillingRoutine();
          break;
          
        case METER_ROUTINE_MEASURING_CHAR:
          selectedMeter.setMeasuringRoutine();
          break;
          
        case METER_ROUTINE_BECAME_EMPTY_CHAR:
          selectedMeter.setBecameEmptyRoutine();
          break;
        
        default:
          Serial.println("ERROR: Invalid routine type.");
          return; 
      }
      
      Serial.println("SUCCESS: Routine set.");
      break;
    }
    
    default:
      Serial.println("ERROR: Command not found.");
      return;
  }
}

// Output a padded float with the given precision/decimals (the padding width is only for the non-decimal digits)
void KegMeterProtocol::PrintWithZeroPadding(float number, byte nonDecimalWidth, int precision) {
  int currentMax = 10;
  for (byte i = 1; i < nonDecimalWidth; i++){
    if (number < currentMax) {
      Serial.print("0");
    }
    currentMax *= 10;
   } 
  Serial.print(number, precision);
}

void KegMeterProtocol::PrintWithZeroPadding(int number, byte width) {
  int currentMax = 10;
  for (byte i = 1; i < width; i++){
    if (number < currentMax) {
      Serial.print("0");
    }
    currentMax *= 10;
   } 
  Serial.print(number);
}

boolean KegMeterProtocol::WaitForAvailable(int numBytes) {
  unsigned long maxWaitTime = millis() + 100;
  while (Serial.available() < numBytes && millis() < maxWaitTime) {} 
  
  return Serial.available() >= numBytes;
}
