#include <Adafruit_NeoPixel.h>
#include "keg_load_meter.h"
#include "keg_meter_protocol.h"

#define LED_OUTPUT_PIN 5
#define EMPTY_CAL_BUTTON_INPUT_PIN 2

#define NUM_KEGS 1
#define NUM_LEDS (NUM_KEGS * KegLoadMeter::NUM_LEDS_PER_METER)

#define DEFAULT_DELAY_TICK_MS 1
#define NUM_LOOPS_WAIT_FOR_OUTPUT 100

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT_PIN, NEO_GRB + NEO_KHZ800);
// NOTE!!!! If you add more strips you will need to render/show them at the end of the main loop!!!

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Each keg will require an instance of the KegLoadMeter object, these will
// each be updated with mass/weight data from sensors for that keg and will show
// the appropriate animations/metering in a robust manor based on sensor calibration

// NOTE: 10 Ohm resistor over the Rg of the amplifier makes for a decent resolution.
// Each 1 integer increment in the incoming analog read value is approximately 1kg.
// Values start around approx 12 -- this should be calibrated for though.

KegLoadMeter kegMeters[] = { KegLoadMeter(0, strip) };
float kegLoadsInKg[NUM_KEGS];
int kegInputPins[] = { 0 }; // Analog input pins for each of the kegs

void setup() {
  Serial.begin(9600);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  // Initialize loads to zero
  for (int kegIdx = 0; kegIdx < NUM_KEGS; kegIdx++) {
    kegLoadsInKg[kegIdx] = 0;
  }
}

void loop() {
  // Keg meter modes are set via serial
  KegMeterProtocol::ReadSerial(kegMeters, NUM_KEGS);
  
  // Perform sensor readings, send them out over serial...
  for (uint8_t kegIdx = 0; kegIdx < NUM_KEGS; kegIdx++) {
    getLoadSensorReading(kegIdx, &kegLoadsInKg[kegIdx]);
  }
  writeKegMeterData(kegLoadsInKg, NUM_LOOPS_WAIT_FOR_OUTPUT);
  
  // Perform sensor readings and meter updates...
  for (int kegIdx = 0; kegIdx < NUM_KEGS; kegIdx++) {
    // Meter update...
    kegMeters[kegIdx].tick(DEFAULT_DELAY_TICK_MS);
  }
  
  // All delays and redraw (i.e., "show") of the strip is done at the end of a frame
  strip.show();
  delay(DEFAULT_DELAY_TICK_MS);
}

/**
 * Get incoming data from the load sensor for the given keg index.
 */
void getLoadSensorReading(uint8_t kegIdx, float* loadValueInKg) {
  // Perform a running average to smooth the readings a little bit
  *loadValueInKg  = 0.99 * (*loadValueInKg) + 0.01 * analogRead(kegInputPins[kegIdx]);
}

void writeKegMeterData(const float* loadValueInKg, int numWaitLoops) {
  static int countLoops = 0;
  
  float currLoad = 0;
  if (countLoops % numWaitLoops == 0) {
    for (int kegIdx = 0; kegIdx < NUM_KEGS; kegIdx++) {
      if (kegMeters[kegIdx].inOutputMeasurementRoutine()) {  
        currLoad = max(0, min(999.999, kegLoadsInKg[kegIdx]));
        KegMeterProtocol::PrintMeasurementMsg(kegIdx, currLoad);
      }
    }
    Serial.println();
    Serial.flush();
    countLoops = 0;
  }
  
  countLoops++;
}






