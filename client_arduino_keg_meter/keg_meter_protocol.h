#ifndef KEG_METER_PROTOCOL_H_
#define KEG_METER_PROTOCOL_H_

#include <Arduino.h>

#define PKG_START_STR "["
#define PKG_END_STR "]"
#define METER_ID_SEPARATOR_STR " "


class KegLoadMeter;

class KegMeterProtocol {
public:
  static void PrintMeasurementMsg(uint8_t meterIdx, float measurement);
  static void ReadSerial(KegLoadMeter* kegMeters, int numMeters);

private:
  KegMeterProtocol() {}
  ~KegMeterProtocol() {}

  static void PrintKegNumberStr(uint8_t meterId) { PrintWithZeroPadding(meterId, 2); } 
  static void PrintStartPkg() { Serial.print(PKG_START_STR); }
  static void PrintEndPkg() { Serial.print(PKG_END_STR); }
  
  static void PrintWithZeroPadding(float number, byte nonDecimalWidth, int precision);
  static void PrintWithZeroPadding(int number, byte width);
  
  static boolean WaitForAvailable(int numBytes);
};

#endif // KEG_METER_PROTOCOL_H_
