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
#define NUM_KEG_METERS 1
KegLoadMeter kegMeters[] = { KegLoadMeter(0, strip) };

void setup() {
  Serial.begin(9600);
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

//int count = 0;
void loop() {
  
  readSerialCommands();
  
  
  kegMeters[0].testTick(10, 1);
  
  //kegMeters[0].setMeterPercentage(((float)i)/(float)(strip.numPixels()));
  //count++;

  //kegMeters[0].showEmptyAnimation(500, 4);
  kegMeters[0].showCalibratingAnimation(20, 1.0);
  //kegMeters[0].showCalibratedAnimation(25);
  
  
  // TODO:
  // All delays and redraw (i.e., "show") of the strip is done at the end of a frame
  strip.show();
  delay(10);
}

// Checks for available serial data -- this can guide certain operations for the meters, including
// calibration of the load sensors when no keg is placed on them ("empty calibration")
// Empty calibration message (all meters): '|Ea'
// Empty calibration message (specific meter): '|Esx', where 'x' is the zero-based index of the meter
// Keg type message (all meters): '|Tay', where 'y' is the type: 0 for corny keg, and 1 for 50L sankey keg
// Keg type message (specific meter): '|Tsxy', where 'x' is the zero-based index of the meter, and 'y' is the type
void readSerialCommands() {

  if (Serial.available() >= 3) {
    boolean commandFailed = false;
    
    char serialReadByte = Serial.read();
    if (serialReadByte == '|') {
      
      // Read the command
      serialReadByte = Serial.read();
      switch (serialReadByte) {
        
        case 'E':
          serialReadByte = Serial.read();
          if (serialReadByte == 'a') {
            // Empty calibration for all meters
            Serial.println("Performing Empty Calibration on all kegs...");
            for (int i = 0; i < NUM_KEG_METERS; i++) {
              kegMeters[i].doEmptyCalibration();
            }
          }
          else if (serialReadByte == 's') {
            // Empty calibration for a specific meter...
            
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);
            
            // Get the meter index to calibrate for...
            int meterIdx = Serial.parseInt();
            if (meterIdx < NUM_KEG_METERS && meterIdx >= 0) {
              Serial.print("Performing Empty Calibration on keg index ");
              Serial.print(meterIdx);
              Serial.println("...");
              kegMeters[meterIdx].doEmptyCalibration();
            }
            else {
              commandFailed = true;
              Serial.print("Command failed, no keg found with index ");
              Serial.println(meterIdx);
            }
          }
          else {
            commandFailed = true;
            Serial.println("Command failed, option not found.");
          }
          break;
          
        case 'T':
          serialReadByte = Serial.read();
          if (serialReadByte == 'a') {
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);
            int kegType = Serial.parseInt();
            for (int i = 0; i < NUM_KEG_METERS; i++) {
              setKegType(kegMeters[i], kegType);
            }
          }
          else if (serialReadByte == 's') {
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);
            
            // Get the meter index
            int meterIdx = Serial.parseInt();
            if (meterIdx < NUM_KEG_METERS && meterIdx >= 0) {
              // There should be another part of the message, wait for it
              while (Serial.available() == 0);
              setKegType(kegMeters[meterIdx], Serial.parseInt());
            }
            else {
              commandFailed = true;
              Serial.print("Command failed, no keg found with index ");
              Serial.println(meterIdx);
            }
          }
          else {
            commandFailed = true;
            Serial.println("Command failed, option not found.");
          }
          
          break;
          
        default:
          commandFailed = true;
          Serial.println("No such command was found.");
          break;    
      }
    }
    else {
      commandFailed = true;
      Serial.println("Commands must begin with '|'.");
    }
    
    if (commandFailed) {
      while (Serial.available() > 0 && Serial.peek() != '|') { Serial.read(); }
    }
  }
}

void setKegType(KegLoadMeter& kegMeter, int kegType) {
  switch (kegType) {
    case 0:
      kegMeter.setKegType(KegLoadMeter::Corny);
      printKegTypeSetMsg(kegMeter, "Corny");
      break;
    case 1:
      kegMeter.setKegType(KegLoadMeter::Sanke50L);
      printKegTypeSetMsg(kegMeter, "Sanke 50L");
      break;
    default:
      Serial.print("Command failed, keg type not found.");
      break;
  }
}

void printKegTypeSetMsg(const KegLoadMeter& kegMeter, const char* typeName) {
  Serial.print("Keg ");
  Serial.print(kegMeter.getIndex());
  Serial.print(" set to ");
  Serial.println(typeName);
}

