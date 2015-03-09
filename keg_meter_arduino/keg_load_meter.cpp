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

const uint8_t KegLoadMeter::ALIVE_BRIGHTNESS = 80;
const uint8_t KegLoadMeter::DEAD_BRIGHTNESS  = 1;
const uint8_t KegLoadMeter::DEATH_PULSE_BRIGHTNESS = 50;
const uint8_t KegLoadMeter::BASE_CALIBRATING_BRIGHTNESS = 80;

#define MIN_EMPTY_CAL_TIME_MS 3000

#define AVG_EMPTY_CORNY_KEG_MASS_KG 4.4
#define AVG_EMPTY_50L_KEG_MASS_KG   13.5

#define MAX_FULL_CORNEY_KEG_MASS_KG (AVG_EMPTY_CORNY_KEG_MASS_KG + 21)

#define MINIMUM_VARIANCE_TO_FINISH_CALIBRATING (0.01)
#define MIN_TRUSTWORTHY_VARIANCE_WHILE_MEASURING (0.1)

static float LERP(float x, float x0, float x1, float y0, float y1) {
  return (y0 + (y1-y0)*(x-x0)/(x1-x0));
}

// This needs to be a bit lighter than the lightest empty keg in use
#define EMPTY_TO_CALIBRATING_MASS (AVG_EMPTY_CORNY_KEG_MASS_KG + 5)

#define CALIBRATE_ANIM_DELAY_MS 5
#define EMPTY_ANIM_PULSE_MS 100
#define NUM_EMPTY_PULSES 5

//#define EEPROM_NUM_ADDRS 5
//#define EEPROM_CAL_EMPTY_AMT_ADDR  (this->meterIdx * EEPROM_NUM_ADDRS + 0);
//#define EEPROM_CAL_FULL_AMT_ADDR   (this->meterIdx * EEPROM_NUM_ADDRS + 1);
//#define EEPROM_EMPTY_KEG_MASS_ADDR (this->meterIdx * EEPROM_NUM_ADDRS + 2);

KegLoadMeter::KegLoadMeter(uint8_t meterIdx, Adafruit_NeoPixel& strip) : 
  meterIdx(meterIdx), startLEDIdx(meterIdx*NUM_LEDS_PER_METER), loadWindowIdx(0), 
  calibratingAnimLEDIdx(0), calibratedAnimLEDIdx(0), currState(Empty), strip(strip), 
  calibratedEmptyLoadAmt(0), delayCounterMillis(0), 
  dataCounter(0), detectedEmptyKegMass(AVG_EMPTY_CORNY_KEG_MASS_KG) {
  
  // Fill the load window with empty data
  this->fillLoadWindow(0.0);
}

void KegLoadMeter::setKegType(KegType kegType) {
  switch (kegType) {
    case Corny:
      this->detectedEmptyKegMass = AVG_EMPTY_CORNY_KEG_MASS_KG;
      break;
      
    case Sanke50L:
      this->detectedEmptyKegMass = AVG_EMPTY_50L_KEG_MASS_KG;
      break;
      
    default:
      break;
  }
}

void KegLoadMeter::setStateValues(float percent, float fullAmt, float emptyAmt) {
  // Fill the window with the value...
  float currMass = fullAmt * percent;
  for (int i = 0; i < LOAD_WINDOW_SIZE; i++) {
    this->loadWindow[i] = currMass;
  }
  this->loadWindowSum = currMass * LOAD_WINDOW_SIZE;
  this->loadWindowVariance = 0;
  
  this->setState(Measuring);
  this->lastPercentAmt = percent;
  this->calibratedFullLoadAmt  = fullAmt;
  this->calibratedEmptyLoadAmt = emptyAmt;
}

void KegLoadMeter::tick(uint32_t frameDeltaMillis, float approxLoadInKg) {
  this->putInLoadWindow(approxLoadInKg);
  
  switch (this->currState) {
    
    case EmptyCalibration: {
      this->turnOff();
      
      this->dataCounter++;
      
      // We fill the load window and find the average "empty" value
      if (this->delayCounterMillis >= MIN_EMPTY_CAL_TIME_MS && this->dataCounter >= LOAD_WINDOW_SIZE) {
        this->calibratedEmptyLoadAmt = this->getLoadWindowMean();
        Serial.println("Empty Calibration Complete.");
        DEBUG_WITH_STR_INT("Calibrated Empty Amount: ", this->calibratedEmptyLoadAmt);        
        this->setState(Empty);
      }
    
      break;
    }
    
    case Empty:
      // LEDs are all turned off
      this->turnOff();

      #ifdef _DEBUG
      static int COUNTER = 0;
      if (COUNTER % 1000 == 0) {        
        DEBUG_WITH_STR_INT("Current load window mean: ", this->getLoadWindowMean());
        DEBUG_WITH_STR_INT("Current load value: ", approxLoadInKg);
        DEBUG_WITH_STR_INT("Empty to calibrating minimum mass: ", this->getEmptyToCalMinMass());
        DEBUG_WITH_STR_INT("Current load window variance: ", this->loadWindowVariance);
        COUNTER = 0;
      }
      COUNTER++;
      #endif

      // Waiting until someone puts a new full/partially-full keg on the sensor...
      if (abs(this->loadWindowVariance) <= MINIMUM_VARIANCE_TO_FINISH_CALIBRATING && 
          this->getLoadWindowMean() >= this->getEmptyToCalMinMass()) {
            
        this->setState(Calibrating);
      }
      break;
      
    case Calibrating: {
      this->dataCounter++;
       
#ifdef _DEBUG
      static int COUNTER = 0;
      if (COUNTER % 100 == 0) {
        DEBUG_WITH_STR_INT("Current load window mean: ", this->getLoadWindowMean());
        DEBUG_WITH_STR_INT("Current load window variance: ", this->loadWindowVariance);
        DEBUG_WITH_STR_INT("Current load value: ", approxLoadInKg);
      }
      COUNTER++;
#endif 
      
      // Check to see if the load goes back below the "empty" threshold
      if (this->getLoadWindowMean() < this->getEmptyToCalMinMass()) {
        this->setState(Empty);
      }
      else {
        // Wait until the variance goes below a certain threshold...
        float percentCalibrated = max(0.0, min(1.0, LERP(abs(this->loadWindowVariance), MINIMUM_VARIANCE_TO_FINISH_CALIBRATING, 1, 1.0, 0.0)));
        float highestPercentCalibrated = min(percentCalibrated, LERP(this->dataCounter, 10*LOAD_WINDOW_SIZE, 1, 1.0, 0.0));
        this->showCalibratingAnimation(CALIBRATE_ANIM_DELAY_MS, highestPercentCalibrated, true);
        
        if (percentCalibrated >= 1.0 && this->dataCounter >= 10*LOAD_WINDOW_SIZE) {
          this->setState(Calibrated); 
        }
      }
      break;
    }
    
    case Calibrated:
      // Be robust -- if for some reason the calibrated amount is "empty" then we need to start over
      if (this->calibratedFullLoadAmt < this->getEmptyToCalMinMass() || this->getLoadWindowMean() < this->calibratedFullLoadAmt*0.9) {
        this->setState(Empty);
      }
      else {
        if (this->showCalibratedAnimation(CALIBRATE_ANIM_DELAY_MS)) {
          // Finished with the animation, on to keeping tabs on the measurement
          this->lastPercentAmt = 1.0;
          this->setState(Measuring); 
        }
      }
      break;
      
    case Measuring: {
 
      float currPercentAmt = this->lastPercentAmt;
      float tempPercentAmt =  max(0.0, min(1.0, LERP(this->getLoadWindowMean(),
        (this->calibratedEmptyLoadAmt + this->detectedEmptyKegMass), this->calibratedFullLoadAmt, 0.0, 1.0)));
        
      if (abs(this->loadWindowVariance) <= MIN_TRUSTWORTHY_VARIANCE_WHILE_MEASURING || (currPercentAmt-tempPercentAmt) <= 0.001) {
        // The meter is being set by the current load amount based on a linear interpolation between
        // the initial calibrated full load and a reasonable "zero" load
        currPercentAmt = tempPercentAmt;
        
        // Running average to make sure we don't overreact to small changes in the mass
        currPercentAmt = 0.99*this->lastPercentAmt + 0.01*currPercentAmt;
        if (currPercentAmt > this->lastPercentAmt) {
          currPercentAmt = this->lastPercentAmt;
        }
        
        this->lastPercentAmt = currPercentAmt;
      }
      
      #ifdef _DEBUG
      static int COUNTER = 0;
      if (COUNTER % 500 == 0) {
        //DEBUG_WITH_STR_INT("Empty window value: ", this->calibratedEmptyLoadAmt);
        //DEBUG_WITH_STR_INT("Current load window mean: ", this->getLoadWindowMean());
        //DEBUG_WITH_STR_INT("Current load window variance: ", this->loadWindowVariance);
        //DEBUG_WITH_STR_INT("Current load value: ", approxLoadInKg);
        Serial.print("Variance: "); Serial.println(this->loadWindowVariance);
        COUNTER = 0;
      }
      COUNTER++;
      #endif

      this->setMeterPercentage(currPercentAmt);
      if (currPercentAmt < 0.011) {
        this->lastPercentAmt = 0.0;
        this->setState(JustBecameEmpty); 
      }

      break;
    }
    case JustBecameEmpty:
      this->dataCounter++;
      
      if (this->showEmptyAnimation(EMPTY_ANIM_PULSE_MS, NUM_EMPTY_PULSES) && this->dataCounter >= LOAD_WINDOW_SIZE) {
        // We need to know what an empty with keg load is so that we know when the keg has been replaced with something more massful
        this->setState(Empty);
      }
      break;
      
    default:
      assert(false);
      return; 
  }
  
  static int OUTPUT_COUNTER = 0;
  if (OUTPUT_COUNTER % 1000 == 0) {
    this->outputStatusToSerial();
    OUTPUT_COUNTER = 0;
  }
  OUTPUT_COUNTER++;
  
  this->delayCounterMillis += frameDeltaMillis;
}

void KegLoadMeter::setState(State newState) {
  
  switch (newState) {
    
    case EmptyCalibration:
      this->dataCounter = 0;
      this->delayCounterMillis = 0;
      this->lastPercentAmt = 0;
      this->calibratedFullLoadAmt = 0;
      this->outputStatusToSerial();
      Serial.println("Entering Empty Calibration State");
      break;
      
    case Empty:
      Serial.println("Entering Empty State");
      this->dataCounter = 0;
      this->lastPercentAmt = 0;
      this->calibratedFullLoadAmt = 0;
      this->turnOff();
      this->outputStatusToSerial();
      break;
      
    case Calibrating:
      Serial.println("Entering Calibrating State");
      this->dataCounter = 0;
      
      // In this state we will be playing the calibration animation
      this->calibratingAnimLEDIdx = 0;
      this->calibratedFullLoadAmt = 0;
      this->outputStatusToSerial();
      break;
      
    case Calibrated:
      Serial.println("Entering Calibrated State");
      this->dataCounter = 0;
      this->calibratedAnimLEDIdx  = 0;
      this->calibratedFullLoadAmt = this->getLoadWindowMean();
      DEBUG_WITH_STR_INT("Full load amount: ", this->calibratedFullLoadAmt);
      
      // Based on the full load amount we can get a pretty good guess at the type of keg
      if (this->calibratedFullLoadAmt > MAX_FULL_CORNEY_KEG_MASS_KG) {
        this->detectedEmptyKegMass = AVG_EMPTY_50L_KEG_MASS_KG;
      }
      else {
        this->detectedEmptyKegMass = AVG_EMPTY_CORNY_KEG_MASS_KG;
      }
      
      this->outputStatusToSerial();
      break;
      
    case Measuring:
      Serial.println("Entering Measuring State");
      this->dataCounter = 0;
      break;
      
    case JustBecameEmpty:
      Serial.println("Entering Just Became Empty State");
      this->dataCounter = 0;
      this->emptyAnimPulseCount = 0;
      this->lastPercentAmt = 0;
      this->calibratedFullLoadAmt = 0;
      this->outputStatusToSerial();
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
void KegLoadMeter::setMeterPercentage(float percent, boolean drawEmptyLEDs) {
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
  this->setMeterPercentage(((float)this->calibratedAnimLEDIdx)/((float)strip.numPixels()), false);
  
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

void KegLoadMeter::outputStatusToSerial() const {
  KegMeterProtocol::OutputStatusMsg(this->getIndex(), this->calibratedFullLoadAmt, 
    this->calibratedEmptyLoadAmt, this->lastPercentAmt, this->getLoadWindowMean(), this->loadWindowVariance);
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

float KegLoadMeter::getEmptyToCalMinMass() const { 
  return this->calibratedEmptyLoadAmt + EMPTY_TO_CALIBRATING_MASS;
}

