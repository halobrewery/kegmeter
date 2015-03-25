#include "keg_load_meter.h"
#include "keg_meter_protocol.h"
#include "assert.h"

//#define _DEBUG
#ifdef _DEBUG
#define DEBUG_WITH_STR_INT(s, i) Serial.print(s); Serial.print(i, DEC); Serial.println()
#define DEBUG_STR(s) Serial.println(s)
#else
#define DEBUG_WITH_STR_INT(s, i)
#define DEBUG_STR(s)
#endif

const uint8_t KegLoadMeter::NUM_RINGS_PER_METER    = 5;
const uint8_t KegLoadMeter::NUM_LEDS_PER_RING      = 16;
const uint8_t KegLoadMeter::HALF_NUM_LEDS_PER_RING = NUM_LEDS_PER_RING/2;
const uint8_t KegLoadMeter::NUM_LEDS_PER_METER     = NUM_RINGS_PER_METER*NUM_LEDS_PER_RING;

const uint8_t KegLoadMeter::ALIVE_BRIGHTNESS = 90;
const uint8_t KegLoadMeter::DEAD_BRIGHTNESS  = 1;
const uint8_t KegLoadMeter::DEATH_PULSE_BRIGHTNESS = 90;
const uint8_t KegLoadMeter::BASE_CALIBRATING_BRIGHTNESS = 70;

#define CALIBRATE_ANIM_DELAY_MS 5
#define EMPTY_ANIM_PULSE_MS 100
#define NUM_EMPTY_PULSES 5

KegLoadMeter::KegLoadMeter(uint8_t meterIdx, Adafruit_NeoPixel& strip) : 
  meterIdx(meterIdx), startLEDIdx(0),
  calibratingAnimLEDIdx(0), calibratedAnimLEDIdx(0), 
  currRoutine(OffRoutine), strip(strip), delayCounterMillis(0) {
  
}

void KegLoadMeter::tick(uint32_t frameDeltaMillis) {

  switch (this->currRoutine) {
    
    case OffRoutine:
      this->turnOff();
      break;
  
    case CalibratingRoutine:      
      this->showCalibratingAnimation(CALIBRATE_ANIM_DELAY_MS, this->fillPercent, true);
      break;

    case FillingRoutine:
      if (this->showCalibratedAnimation(CALIBRATE_ANIM_DELAY_MS)) {
          // Finished with the filling animation, next state is measurement
          this->setRoutine(MeasuringRoutine); 
      }
      break;
    
    case MeasuringRoutine:
      this->showPercentage(this->fillPercent);
      break;

    case BecameEmptyRoutine:
      if (this->showEmptyAnimation(EMPTY_ANIM_PULSE_MS, NUM_EMPTY_PULSES)) {
        // We need to know what an empty with keg load is so that we know when the keg has been replaced with something more massful
        this->setRoutine(OffRoutine);
      }
      break;
      
    default:
      assert(false);
      return; 
  }
  
  this->delayCounterMillis += frameDeltaMillis;
}

void KegLoadMeter::setRoutine(Routine newRoutine) {
  
  switch (newRoutine) {
    
    case OffRoutine:
      Serial.println("Entering 'Off' Routine.");
      this->turnOff();
      break;
      
    case CalibratingRoutine:
      Serial.println("Entering 'Calibrating' Routine.");
      this->calibratingAnimLEDIdx = 0;
      break;
    
    case FillingRoutine:
      Serial.println("Entering 'Filling' Routine.");
      this->calibratedAnimLEDIdx  = 0;
      break;
      
    case MeasuringRoutine:
      Serial.println("Entering 'Measuring' Routine.");
      break;
      
    case BecameEmptyRoutine:
      Serial.println("Entering 'Became Empty' Routine.");
      this->emptyAnimPulseCount = 0;
      break;
      
    default:
      assert(false);
      return;
  }
  
  this->currRoutine = newRoutine;
}

/**
 * Show the keg meter's percentage on the LED rings.
 * Params:
 * percent - The percentage to set [0,1].
 */
void KegLoadMeter::showPercentage(float percent, boolean drawEmptyLEDs) {
  uint16_t idx = this->startLEDIdx;
  uint16_t litEndIdx = idx + ceil(percent * KegLoadMeter::NUM_LEDS_PER_METER);
  uint16_t meterEndIdx = idx + KegLoadMeter::NUM_LEDS_PER_METER;

  for (; idx < litEndIdx; idx++) {
    this->strip.setPixelColor(idx, getFullColour());
  }
  
  if (drawEmptyLEDs) {
    idx = litEndIdx;
    for (; idx < meterEndIdx; idx++) {
      this->strip.setPixelColor(idx, getEmptyColour());
    }
  }
}

boolean KegLoadMeter::showEmptyAnimation(uint8_t pulseTimeInMillis, uint8_t numPulses) {

  uint16_t meterEndIdx = this->startLEDIdx + KegLoadMeter::NUM_LEDS_PER_METER;  
  uint32_t deathColour = getEmptyAnimationColour(this->emptyAnimPulseCount);
  for (uint16_t idx = this->meterIdx * KegLoadMeter::NUM_LEDS_PER_METER; idx <= meterEndIdx; idx++) {
    this->strip.setPixelColor(idx, deathColour);
  }
  
  if (this->delayCounterMillis >= pulseTimeInMillis) {
    this->emptyAnimPulseCount++;
    this->delayCounterMillis = 0;
  }
  
  if (this->emptyAnimPulseCount > 2*numPulses) {
    return true;
  }
  
  return false;
}

/**
 * Shows a calibration animation for the given meter based on the
 * given percentage calibrated.
 * Params:
 * percentCalibrated - The percentage that the calibration is complete in [0,1]. 0 being 
 * not calibrated at all, 1 being fully calibrated.
 */
void KegLoadMeter::showCalibratingAnimation(uint8_t delayMillis, float percentCalibrated, boolean resetDelayCounter) {

  static const uint8_t OFFSET = 4;
  static const uint8_t TRAIL_SIZE = 9;
  
  this->turnOff();
  
  // Draw spinning loading wheels on each of the rings...
  const uint8_t mostBright = (uint8_t)max(TRAIL_SIZE, percentCalibrated * BASE_CALIBRATING_BRIGHTNESS);
  for (int ringIdx = 0; ringIdx < NUM_RINGS_PER_METER; ringIdx++) {
    int16_t currRingStartLEDIdx = this->startLEDIdx + ringIdx * NUM_LEDS_PER_RING;
    int16_t currRingEndLEDIdx = currRingStartLEDIdx + NUM_LEDS_PER_RING;
    int16_t currLEDIdx = currRingStartLEDIdx + this->calibratingAnimLEDIdx;
    
    // Bright pixel and a trail...
    for (uint8_t trailIdx = 0; trailIdx < TRAIL_SIZE; trailIdx++) {
      int16_t mappedLEDIdx = currLEDIdx - trailIdx;
      if (mappedLEDIdx < currRingStartLEDIdx) {
        mappedLEDIdx += NUM_LEDS_PER_RING;
      }
      else if (mappedLEDIdx > currRingEndLEDIdx) {
        mappedLEDIdx -= NUM_LEDS_PER_RING;
      }
      
      float diminishAmt = pow(((float)trailIdx) / ((float)TRAIL_SIZE), 0.6);
      this->strip.setPixelColor(mappedLEDIdx, this->getDiminishedWhite(1.0-diminishAmt, mostBright));
    }
  }
  
  if (this->delayCounterMillis >= delayMillis) {
    this->calibratingAnimLEDIdx = (this->calibratingAnimLEDIdx + 1) % NUM_LEDS_PER_RING;
    if (resetDelayCounter) {
      this->delayCounterMillis = 0;
    }
  }
}

boolean KegLoadMeter::showCalibratedAnimation(uint8_t delayMillis) {
  this->showCalibratingAnimation(delayMillis, 1.0, false);
  
  // Start filling the meter
  this->showPercentage(((float)this->calibratedAnimLEDIdx)/((float)strip.numPixels()), false);
  
  if (this->delayCounterMillis >= delayMillis) {
    this->calibratedAnimLEDIdx++;
    this->delayCounterMillis = 0;
  }
   
  if (this->calibratedAnimLEDIdx == this->strip.numPixels()) {
    return true;
  }

  return false;
}

void KegLoadMeter::turnOff() {
  uint16_t idx = this->startLEDIdx;
  uint16_t meterEndIdx = idx + KegLoadMeter::NUM_LEDS_PER_METER;
  for (; idx < meterEndIdx; idx++) {
    this->strip.setPixelColor(idx, 0);
  }
}

uint32_t KegLoadMeter::getEmptyAnimationColour(uint8_t cycleIdx) const {
  if (cycleIdx % 2 == 0) {
    return this->strip.Color(0,0,0);
  }
  return this->strip.Color(DEATH_PULSE_BRIGHTNESS, 0, 0);
}

uint32_t KegLoadMeter::getCalibratingColour(float percentCalibrated) const {
  return this->getDiminishedWhite(percentCalibrated, ALIVE_BRIGHTNESS);
}

uint32_t KegLoadMeter::getDiminishedWhite(float multiplier, uint8_t whiteAmt) const {
  uint8_t colourVal = multiplier*whiteAmt;
  return this->strip.Color(colourVal,colourVal,colourVal);
}

uint32_t KegLoadMeter::getDiminishedColour(float multiplier, uint8_t r, uint8_t g, uint8_t b) const {
  return this->strip.Color(multiplier*r, multiplier*g, multiplier*b);
}

