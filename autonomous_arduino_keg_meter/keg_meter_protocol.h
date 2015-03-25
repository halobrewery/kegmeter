#ifndef KEG_METER_PROTOCOL_H_
#define KEG_METER_PROTOCOL_H_

#include <Arduino.h>

#define KEG_NUMBER 1

class KegMeterProtocol {
public:
  static void OutputMeasuredPercentMsg(uint8_t meterIdx, float percent) { 
    OutputStartPkg();
    OutputKegNumberStr(meterIdx);
    Serial.print("{P:"); Serial.print(percent); Serial.print("}"); 
    OutputEndPkg();
  }

  static void OutputStatusMsg(uint8_t meterIdx, float fullMass, float emptyMass, float percent, float load, float variance) {
    OutputStartPkg();
    OutputKegNumberStr(meterIdx);
    Serial.print("{P:"); Serial.print(percent, 2); 
    Serial.print(",F:"); Serial.print(fullMass, 2);
    Serial.print(",E:"); Serial.print(emptyMass, 2);
    Serial.print(",L:"); Serial.print(load, 2);
    Serial.print(",V:"); Serial.print(variance, 5);
    Serial.print("}");
    OutputEndPkg();
  }
  
private:
  KegMeterProtocol() {}
  ~KegMeterProtocol() {}
  
  static void OutputKegNumberStr(uint8_t meterId) { Serial.print(meterId); } 
  static void OutputStartPkg() { Serial.print("["); }
  static void OutputEndPkg() { Serial.println("]"); }
  
};

#endif // KEG_METER_PROTOCOL_H_
