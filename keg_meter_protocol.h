#ifndef KEG_METER_PROTOCOL_H_
#define KEG_METER_PROTOCOL_H_

#include <Arduino.h>

#define KEG_NUMBER 1

class KegMeterProtocol {
public:
  static void OutputMeasuredPercentMsg(float percent) { OutputKegNumberStr(); Serial.print("{M,"); Serial.print(percent); Serial.println("}"); }
  static void OutputEmptyMsg() { OutputKegNumberStr(); Serial.println("{E}"); }
  
private:
  KegMeterProtocol() {}
  ~KegMeterProtocol() {}
  
  static void OutputKegNumberStr() { Serial.print("["); Serial.print(KEG_NUMBER); Serial.print("]"); } 
};

#endif // KEG_METER_PROTOCOL_H_
