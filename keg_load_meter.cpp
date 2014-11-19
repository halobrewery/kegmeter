#include "keg_load_meter.h"
#include "assert.h"

const uint8_t KegLoadMeter::NUM_RINGS_PER_METER    = 5;
const uint8_t KegLoadMeter::NUM_LEDS_PER_RING      = 16;
const uint8_t KegLoadMeter::HALF_NUM_LEDS_PER_RING = NUM_LEDS_PER_RING/2;
const uint8_t KegLoadMeter::NUM_LEDS_PER_METER     = NUM_RINGS_PER_METER*NUM_LEDS_PER_RING;

const uint8_t KegLoadMeter::ALIVE_BRIGHTNESS = 60;
const uint8_t KegLoadMeter::DEAD_BRIGHTNESS  = 2;
const uint8_t KegLoadMeter::DEATH_PULSE_BRIGHTNESS = 50;
const uint8_t KegLoadMeter::BASE_CALIBRATING_BRIGHTNESS = 40;

#define KEG_MASS 13.5
#define EMPTY_TO_CALIBRATING_MASS (KEG_MASS + 2) // This needs to be a bit more massful than a keg

#define MINIMUM_VARIANCE_TO_FINISH_CALIBRATING (0.25 * 0.25)
#define LERP(x, x0, x1, y0, y1) (y0 + (y1-y0) * (x-x0) / (x1-x0))

#define CALIBRATE_ANIM_DELAY_MS 125
#define EMPTY_ANIM_PULSE_MS 500

KegLoadMeter::KegLoadMeter(uint8_t meterIdx, Adafruit_NeoPixel& strip) : 
  meterIdx(meterIdx), startLEDIdx(meterIdx*NUM_LEDS_PER_METER), loadWindowIdx(0), 
  calibratedAnimLEDIdx(0), currState(Empty), strip(strip) {
  
  // Fill the load window with empty data
  this->fillLoadWindow(0.0);
}

// TODO:
// Do a box filter window for about 10 seconds with 100 samples in it... or something.

void KegLoadMeter::updateAndShow(float approxLoadInKg) {
  this->putInLoadWindow(approxLoadInKg);
  
  switch (this->currState) {
    
    case Empty:
      // LEDs are all turned off
      this->turnOff(true);
      
      // TODO: Calibrate what empty is... need to set up the system to do this!!!
      
      // Waiting until someone puts a new full/partially-full keg on the sensor...
      if (this->getLoadWindowMean() >= EMPTY_TO_CALIBRATING_MASS) {
        this->setState(Calibrating);
      }
      break;
      
    case Calibrating:
      // Check to see if the load goes back below the "empty" threshold
      if (this->getLoadWindowMean() < EMPTY_TO_CALIBRATING_MASS) {
        this->setState(Empty);
      }
      else {
        // Wait until the variance goes below a certain threshold...
        float percentCalibrated = max(0.0, min(1.0, LERP(this->loadWindowVariance, MINIMUM_VARIANCE_TO_FINISH_CALIBRATING, 1, 1.0, 0.0)));
        this->showCalibratingAnimation(CALIBRATE_ANIM_DELAY_MS, percentCalibrated);
      
        if (percentCalibrated >= 1.0) {
          this->setState(Calibrated); 
        }
      }
      break;
    
    case Calibrated:
      // Be robust -- if for some reason the calibrated amount is "empty" then we need to start over
      if (this->calibratedFullLoadAmt < EMPTY_TO_CALIBRATING_MASS) {
        this->setState(Empty);
      }
      else {
        if (this->showCalibratedAnimation(CALIBRATE_ANIM_DELAY_MS)) {
          // Finished with the animation, on to keeping tabs on the measurement
         this->setState(Measuring); 
        }
      }
      break;
      
    case Measuring:
      // The meter is being set by the current load amount based on a linear interpolation between
      // the initial calibrated full load and a reasonable "zero" load
      // TODO
      break;
      
    case JustBecameEmpty:
      this->showEmptyAnimation(EMPTY_ANIM_PULSE_MS, 4); // This is "blocking" for the entire animation
      this->setState(Empty);
      break;
    
    default:
      assert(false);
      return; 
  }
}

void KegLoadMeter::setState(State newState) {
  switch (newState) {
    case Empty:
      this->turnOff(true);
      break;
      
    case Calibrating:
      // In this state we will be playing the calibration animation
      break;
      
    case Calibrated:
      this->calibratedAnimLEDIdx  = 0;
      this->calibratedFullLoadAmt = this->getLoadWindowMean();
      break;
      
    case Measuring:
      break;
      
    case JustBecameEmpty:
      break;
    
    default:
      assert(false);
      return;
  }
  
  this->currState = newState;
}

/**
 * Set one of the keg meter's percentages.
 * Params:
 * percent - The percentage to set [0,1].
 */
void KegLoadMeter::setMeterPercentage(float percent, boolean drawEmptyLEDs, boolean doShow) {
  uint16_t idx = this->startLEDIdx;
  uint16_t litEndIdx = idx + ceil(percent * KegLoadMeter::NUM_LEDS_PER_METER);
  uint16_t meterEndIdx = idx + KegLoadMeter::NUM_LEDS_PER_METER;
  
  for (; idx <= litEndIdx; idx++) {
    this->strip.setPixelColor(idx, getFullColour());
  }
  if (drawEmptyLEDs) {
    idx = litEndIdx + 1;
    for (; idx <= meterEndIdx; idx++) {
      this->strip.setPixelColor(idx, getEmptyColour());
    }
  }
  if (doShow) {
    this->strip.show();
  }
}

void KegLoadMeter::showEmptyAnimation(uint8_t pulseTimeInMillis, uint8_t numPulses) {
  uint16_t meterEndIdx = this->startLEDIdx + KegLoadMeter::NUM_LEDS_PER_METER;
  uint16_t pulseCycles = 2*numPulses;
  
  for (uint8_t pulseIdx = 0; pulseIdx < pulseCycles; pulseIdx++) {
    uint32_t deathColour = getEmptyAnimationColour(pulseIdx);
    for (uint16_t idx = this->meterIdx * KegLoadMeter::NUM_LEDS_PER_METER; idx <= meterEndIdx; idx++) {
      this->strip.setPixelColor(idx, deathColour);
    }
    this->strip.show();
    delay(pulseTimeInMillis);
  }
}

/**
 * Shows a calibration animation for the given meter based on the
 * given percentage calibrated.
 * Params:
 * percentCalibrated - The percentage that the calibration is complete in [0,1]. 0 being 
 * not calibrated at all, 1 being fully calibrated.
 */
void KegLoadMeter::showCalibratingAnimation(uint8_t delayMillis, float percentCalibrated, boolean doShowAndDelay) {

  static const uint8_t OFFSET = 4;
  static const uint8_t TRAIL_SIZE = 9;
  static uint8_t SPIN_COUNTER = 0;
  
  this->turnOff(false);
  
  // Draw spinning loading wheels on each of the rings...
  const uint8_t mostBright = (uint8_t)max(TRAIL_SIZE, percentCalibrated * BASE_CALIBRATING_BRIGHTNESS);
  for (int ringIdx = 0; ringIdx < NUM_RINGS_PER_METER; ringIdx++) {
    int16_t currRingStartLEDIdx = this->startLEDIdx + ringIdx * NUM_LEDS_PER_RING;
    int16_t currRingEndLEDIdx = currRingStartLEDIdx + NUM_LEDS_PER_RING;
    int16_t currLEDIdx = currRingStartLEDIdx + SPIN_COUNTER;
    
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
  
  SPIN_COUNTER = (SPIN_COUNTER + 1) % NUM_LEDS_PER_RING;
  
  if (doShowAndDelay) {
    this->strip.show();
    delay(delayMillis);
  }
}

boolean KegLoadMeter::showCalibratedAnimation(uint8_t delayMillis) {
  this->showCalibratingAnimation(0, 1.0, false);
  
  // Start filling the meter
  this->setMeterPercentage(((float)this->calibratedAnimLEDIdx)/((float)strip.numPixels()), false, false);
  
  this->strip.show();
  delay(delayMillis);
 
  if (this->calibratedAnimLEDIdx == this->strip.numPixels()) {
    return true;
  }

  this->calibratedAnimLEDIdx++;
  return false;
}

void KegLoadMeter::turnOff(boolean doShow) {
  uint16_t idx = this->startLEDIdx;
  uint16_t meterEndIdx = idx + KegLoadMeter::NUM_LEDS_PER_METER;
  for (; idx < meterEndIdx; idx++) {
    this->strip.setPixelColor(idx, 0);
  }
  if (doShow) {
    this->strip.show();
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

void KegLoadMeter::fillLoadWindow(float value) {
  for (int i = 0; i < LOAD_WINDOW_SIZE; i++) {
    this->loadWindow[i] = value;
  }
  this->loadWindowSum = value * LOAD_WINDOW_SIZE;
  this->loadWindowVariance = 0.0;
}

void KegLoadMeter::putInLoadWindow(float value) {
  {
    float prevVarianceValue  = (this->loadWindow[this->loadWindowIdx] - this->getLoadWindowMean());
    prevVarianceValue *= prevVarianceValue;
    prevVarianceValue /= (float)LOAD_WINDOW_SIZE;
    
    this->loadWindowVariance -= prevVarianceValue;
  }
  
  this->loadWindowSum -= this->loadWindow[this->loadWindowIdx];
  this->loadWindowSum += value;  
  this->loadWindow[this->loadWindowIdx] = value;
  this->loadWindowIdx = (this->loadWindowIdx + 1) % LOAD_WINDOW_SIZE;
  
  {
    float newVarianceValue = (value - this->getLoadWindowMean());
    newVarianceValue *= newVarianceValue;
    newVarianceValue /= (float)LOAD_WINDOW_SIZE;
    
    this->loadWindowVariance += newVarianceValue;
  }
  
}
