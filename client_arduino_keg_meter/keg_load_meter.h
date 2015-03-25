#ifndef KEG_LOAD_METER_H_
#define KEG_LOAD_METER_H_

#include <Adafruit_NeoPixel.h>

class KegLoadMeter {
public:
  static const uint8_t NUM_RINGS_PER_METER;
  static const uint8_t NUM_LEDS_PER_RING;
  static const uint8_t HALF_NUM_LEDS_PER_RING;
  static const uint8_t NUM_LEDS_PER_METER;

  KegLoadMeter(uint8_t meterIdx, Adafruit_NeoPixel& strip);
  ~KegLoadMeter() {}

  uint8_t getIndex() const { return this->meterIdx; }
  uint8_t getId() const { return this->meterIdx+1; }
  
  boolean inOutputMeasurementRoutine() const;
  
  void tick(uint32_t frameDeltaMillis);
  
  void setOffRoutine();
  void setCalibratingRoutine();
  void setFillingRoutine();
  void setMeasuringRoutine();
  void setBecameEmptyRoutine();

  void setPercentage(float percent);
  
private:
  const uint8_t meterIdx;          // The zero-based index of this meter in the LED strip
  const uint16_t startLEDIdx;
  
  // Stateful members: keep track of information in various states
  uint32_t delayCounterMillis;   // Used across all states for tracking the total delay in ms
  uint8_t calibratingAnimLEDIdx; // State: CalibratingRoutine
  uint16_t calibratedAnimLEDIdx; // State: FillingRoutine
  uint8_t emptyAnimPulseCount;   // State: BecameEmptyRoutine
  float fillPercent;             // Value in [0,1] representing how full the meter should be

  Adafruit_NeoPixel& strip;  // The LED strip object
  
  static const uint8_t ALIVE_BRIGHTNESS;
  static const uint8_t DEAD_BRIGHTNESS;
  static const uint8_t DEATH_PULSE_BRIGHTNESS;
  static const uint8_t BASE_CALIBRATING_BRIGHTNESS;
  
  enum Routine {
    OffRoutine,          // Turned off
    CalibratingRoutine,  // Each of the circles "swirls" as the meter calibrates
    FillingRoutine,      // The meter fills up as the "swirling" LEDs continue from the calibration routine
    MeasuringRoutine,    // This is the "typical" state for showing the current status of the keg as people drink from it 100%->0% on the meter
    BecameEmptyRoutine   // Flash red, keg is empty
  } currRoutine;

  void setRoutine(Routine newRoutine);

  boolean showEmptyAnimation(uint8_t pulseTimeInMillis, uint8_t numPulses);
  void showCalibratingAnimation(uint8_t delayMillis, float percentCalibrated, boolean resetDelayCounter = false);
  boolean showCalibratedAnimation(uint8_t delayMillis);
  
  void turnOff();
  void showPercentage(float percent, boolean drawEmptyLEDs = true);
  
  uint32_t getFullColour() const { return this->strip.Color(ALIVE_BRIGHTNESS, ALIVE_BRIGHTNESS, ALIVE_BRIGHTNESS); }
  uint32_t getEmptyColour() const { return this->strip.Color(DEAD_BRIGHTNESS, DEAD_BRIGHTNESS, DEAD_BRIGHTNESS); }
  uint32_t getEmptyAnimationColour(uint8_t cycleIdx) const;
  uint32_t getCalibratingColour(float percentCalibrated) const;
  uint32_t getDiminishedWhite(float multiplier, uint8_t whiteAmt) const;
  uint32_t getDiminishedColour(float multiplier, uint8_t r, uint8_t g, uint8_t b) const;
};

inline boolean KegLoadMeter::inOutputMeasurementRoutine() const {
  return (this->currRoutine != FillingRoutine && this->currRoutine != BecameEmptyRoutine);
}

inline void KegLoadMeter::setOffRoutine() {
  this->setRoutine(OffRoutine);
}

inline void KegLoadMeter::setCalibratingRoutine() {
  this->setRoutine(CalibratingRoutine);
}

inline void KegLoadMeter::setFillingRoutine() {
  this->setRoutine(FillingRoutine);
}

inline void KegLoadMeter::setMeasuringRoutine() {
  this->setRoutine(MeasuringRoutine);
}

inline void KegLoadMeter::setBecameEmptyRoutine() {
  this->setRoutine(BecameEmptyRoutine);
}

inline void KegLoadMeter::setPercentage(float percent) {
  this->fillPercent = percent;
}

#endif
