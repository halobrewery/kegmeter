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
  
  void updateAndShow(float approxLoadInKg);

  void setMeterPercentage(float percent, boolean drawEmptyLEDs = true, boolean doShow = true);
  void showEmptyAnimation(uint8_t pulseTimeInMillis, uint8_t numPulses);
  void showCalibratingAnimation(uint8_t delayMillis, float percentCalibrated, boolean doShowAndDelay = true);
  boolean showCalibratedAnimation(uint8_t delayMillis);
  
  void turnOff(boolean doShow = true);
  
private:
  const uint8_t meterIdx;          // The zero-based index of this meter in the LED strip
  const uint16_t startLEDIdx;
  
  // Stateful members: keep track of information in various states
  uint16_t calibratedAnimLEDIdx; // State: Calibrated
  float calibratedFullLoadAmt;   // State: Calibrated, Measuring
  
  
  static const int LOAD_WINDOW_SIZE = 50; // Never make this bigger than 255!!
  uint8_t loadWindowIdx;
  float loadWindow[LOAD_WINDOW_SIZE];
  float loadWindowSum;
  float loadWindowVariance;
  
  Adafruit_NeoPixel& strip;  // The LED strip object
  
  enum State {
    Empty,
    Calibrating,
    Calibrated,
    Measuring,
    JustBecameEmpty
  } currState;

  void setState(State newState);
  
  static const uint8_t ALIVE_BRIGHTNESS;
  static const uint8_t DEAD_BRIGHTNESS;
  static const uint8_t DEATH_PULSE_BRIGHTNESS;
  static const uint8_t BASE_CALIBRATING_BRIGHTNESS;
  
  uint32_t getFullColour() const { return this->strip.Color(ALIVE_BRIGHTNESS, ALIVE_BRIGHTNESS, ALIVE_BRIGHTNESS); }
  uint32_t getEmptyColour() const { return this->strip.Color(DEAD_BRIGHTNESS, DEAD_BRIGHTNESS, DEAD_BRIGHTNESS); }
  uint32_t getEmptyAnimationColour(uint8_t cycleIdx) const;
  uint32_t getCalibratingColour(float percentCalibrated) const;
  uint32_t getDiminishedWhite(float multiplier, uint8_t whiteAmt) const;
  
  
  void fillLoadWindow(float value);
  void putInLoadWindow(float value);
  
  float getLoadWindowMean() const { return this->loadWindowSum / ((float)LOAD_WINDOW_SIZE); }
  
};

#endif
