#include <Adafruit_NeoPixel.h>
#include "keg_load_meter.h"

#define OUTPUT_PIN 6

#define NUM_METERS 1
#define NUM_LEDS (NUM_METERS * KegLoadMeter::NUM_LEDS_PER_METER)



// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, OUTPUT_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Each keg will require an instance of the KegLoadMeter object, these will
// each be updated with mass/weight data from sensors for that keg and will show
// the appropriate animations/metering in a robust manor based on sensor calibration
KegLoadMeter kegMeter = KegLoadMeter(0, strip);

// TODO: Calibration procedure (for "empty sensors") -- have a serial protocol for this, we don't
// want to have it happen whenever the system is rebooted or some such not-so-carefully-controlled circumstance



void setup() {
  Serial.begin(9600);
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

//int i = 0;
void loop() {
  
  kegMeter.testTick(10, 1);
  
  
  //kegMeter.setMeterPercentage(((float)i)/(float)(strip.numPixels()));
  //i++;

  
  //kegMeter.showEmptyAnimation(500, 4);
  //kegMeter.showCalibratingAnimation(20, 1.0);
  kegMeter.showCalibratedAnimation(25);
  
  
  // TODO:
  // All delays and redraw (i.e., "show") of the strip is done at the end of a frame
  strip.show();
  delay(10);
}

