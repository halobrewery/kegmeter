#ifndef KEG_LOAD_METER_H_
#define KEG_LOAD_METER_H_

#include <Adafruit_NeoPixel.h>

class KegLoadMeter {
public:
  static const uint8_t NUM_RINGS_PER_METER;
  static const uint8_t NUM_LEDS_PER_RING;
  static const uint8_t HALF_NUM_LEDS_PER_RING;
  static const uint8_t NUM_LEDS_PER_METER;

  enum KegType { Corny, Sanke50L };
  
  KegLoadMeter(uint8_t meterIdx, Adafruit_NeoPixel& strip);
  ~KegLoadMeter() {}

  uint8_t getIndex() const { return this->meterIdx; }

  void doEmptyCalibration() { this->setState(EmptyCalibration); }
  void setKegType(KegType kegType);

  void tick(uint32_t frameDeltaMillis, float approxLoadInKg);

  void setMeterPercentage(float percent, boolean drawEmptyLEDs = true);
  boolean showEmptyAnimation(uint8_t pulseTimeInMillis, uint8_t numPulses);
  void showCalibratingAnimation(uint8_t delayMillis, float percentCalibrated, boolean resetDelayCounter = false);
  boolean showCalibratedAnimation(uint8_t delayMillis);
  
  void turnOff();
  
private:
  const uint8_t meterIdx;          // The zero-based index of this meter in the LED strip
  const uint16_t startLEDIdx;
  
  // Stateful members: keep track of information in various states
  uint16_t dataCounter;
  uint32_t delayCounterMillis;   // Used across all states for tracking the total delay in ms
  uint8_t calibratingAnimLEDIdx; // State: Calibrating
  uint16_t calibratedAnimLEDIdx; // State: Calibrated
  float calibratedFullLoadAmt;   // State: Calibrated, Measuring
  float lastPercentAmt;
  uint8_t emptyAnimPulseCount;
  float calibratedEmptyLoadAmt;  // State: EmptyCalibration, Empty, JustBecameEmpty, Measuring
  float detectedEmptyKegMass;
  
  static const int LOAD_WINDOW_SIZE = 100; // Never make this bigger than 255!!
  uint8_t loadWindowIdx;
  float loadWindow[LOAD_WINDOW_SIZE];
  float loadWindowSum;
  float loadWindowVariance;
  
  Adafruit_NeoPixel& strip;  // The LED strip object
  
  enum State {
    EmptyCalibration, // Calibration of the load sensor for this meter when nothing has been placed on it
    Empty,            // State to rest in when the keg is empty or there is no keg on the sensor for this meter
    Calibrating,      // A keg (or something with mass) has been detected on the sensor and it needs to calibrate for it
    Calibrated,       // Calibration for the keg is complete, the keg is now considered "full"
    Measuring,        // This is the "typical" state for showing the current status of the keg as people drink from it 100%->0% on the meter
    JustBecameEmpty   // Happens when the meter hits ~0%
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
  uint32_t getDiminishedColour(float multiplier, uint8_t r, uint8_t g, uint8_t b) const;
  
  void fillLoadWindow(float value);
  void putInLoadWindow(float value);
  
  float getLoadWindowMean() const { return this->loadWindowSum / ((float)LOAD_WINDOW_SIZE); }
  float getEmptyToCalMinMass() const;
};

#endif
