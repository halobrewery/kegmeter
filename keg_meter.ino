#include <Adafruit_NeoPixel.h>
#include "keg_load_meter.h"

#define LED_OUTPUT_PIN 6
#define EMPTY_CAL_BUTTON_INPUT_PIN 2

#define NUM_KEGS 1
#define NUM_LEDS (NUM_KEGS * KegLoadMeter::NUM_LEDS_PER_METER)

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT_PIN, NEO_GRB + NEO_KHZ800);

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
int kegInputPins[] = { 0 };

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


// Enter your calibrated values here
float loadA = 4.313; // kg
int analogvalA = 168.5; // analog reading taken with load A on the load cells
float loadB = 5.079; // kg 
int analogvalB = 170.4; // analog reading taken with load B on the load cells

float analogToLoad(float analogVal){
  return max(0, mapfloat(analogVal, analogvalA, analogvalB, loadA, loadB));
  //return analogVal;
}


// Simulation defines
#define DEFAULT_DELAY_TICK_MS 1

// Serial protocol defines
#define METER_SELECT_CHAR 'm'
#define ALL_METERS_CHAR 'a'
#define EMPTY_CALIBRATE_MODE_CHAR 'E'
#define KEG_TYPE_CHANGE_CHAR 'T'
#define FILL_LOAD_WINDOW_CHAR 'L'
#define PKG_BEGIN_CHAR '|'
//#define PKG_END_CHAR '~'


void setup() {
  Serial.begin(9600);
  
  pinMode(EMPTY_CAL_BUTTON_INPUT_PIN, INPUT);
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

//int count = 0;
void loop() {
  // Keg modes/calibration can be set via serial...
  readSerialCommands();
  
  if (digitalRead(EMPTY_CAL_BUTTON_INPUT_PIN) >= 1) {
    doEmptyCalibrationToAllKegs();
  }
  
  // Perform sensor readings and meter updates...
  for (uint8_t kegIdx = 0; kegIdx < NUM_KEGS; kegIdx++) {
    
    // Sensor readings...
    getLoadSensorReading(kegIdx, &kegLoadsInKg[kegIdx]);
    
    // Meter update...
    kegMeters[kegIdx].tick(DEFAULT_DELAY_TICK_MS, kegLoadsInKg[kegIdx]);
  }
  
  // All delays and redraw (i.e., "show") of the strip is done at the end of a frame
  strip.show();
  delay(DEFAULT_DELAY_TICK_MS);
}

/**
 * Get incoming data from the load sensor for the given keg and populate the given mass value.
 */
void getLoadSensorReading(uint8_t kegIdx, float* loadValueInKg) {
  
  int analogValue = analogRead(kegInputPins[kegIdx]);
  
  // Perform a running average to smooth the readings a little bit
  *loadValueInKg  = 0.99 * (*loadValueInKg) + 0.01 * analogToLoad(analogValue);
}

void doEmptyCalibrationToAllKegs() {
  Serial.println("Performing Empty Calibration on all meters...");
  for (int i = 0; i < NUM_KEGS; i++) {
    kegMeters[i].doEmptyCalibration();
  }
}

// Checks for available serial data -- this can guide certain operations for the meters, including
// calibration of the load sensors when no keg is placed on them ("empty calibration")
// Empty calibration message (all meters): '|Ea~'
// Empty calibration message (specific meter): '|Emx~', where 'x' is the zero-based index of the meter
// Keg type message (all meters): '|Tay~', where 'y' is the type: 'c' for corny keg, and 's' for 50L sankey keg
// Keg type message (specific meter): '|Tmxy~', where 'x' is the zero-based index of the meter, and 'y' is the type
// Set the loads for all kegs '|Lx~', where 'x' is the floating point value to fill the load window with in kg
void readSerialCommands() {

  if (Serial.available() >= 3) {

    char serialReadByte = Serial.read();
    if (serialReadByte == PKG_BEGIN_CHAR) {
      
      // Read the command
      serialReadByte = Serial.read();
      switch (serialReadByte) {
        
        case EMPTY_CALIBRATE_MODE_CHAR:
          serialReadByte = Serial.read();
          if (serialReadByte == ALL_METERS_CHAR) {
            // Empty calibration for all meters
            doEmptyCalibrationToAllKegs();
          }
          else if (serialReadByte == METER_SELECT_CHAR) {
            // Empty calibration for a specific meter...
            
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);

            // Get the meter index to calibrate for...
            int meterIdx = Serial.parseInt();
            if (meterIdx < NUM_KEGS && meterIdx >= 0) {
              Serial.print("Performing Empty Calibration on keg index ");
              Serial.print(meterIdx);
              Serial.println("...");
              kegMeters[meterIdx].doEmptyCalibration();
            }
            else {
              Serial.print("Command failed, no keg found with index ");
              Serial.println(meterIdx);
            }
          }
          else {
            Serial.println("Command failed, option not found.");
          }
          break;
          
        case KEG_TYPE_CHANGE_CHAR:
          serialReadByte = Serial.read();
          if (serialReadByte == ALL_METERS_CHAR) {
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);
         
            int kegType = Serial.parseInt();
            for (int i = 0; i < NUM_KEGS; i++) {
              setKegType(kegMeters[i], kegType);
            }
          }
          else if (serialReadByte == METER_SELECT_CHAR) {
            // There should be another part of the message, wait for it
            while (Serial.available() == 0);
            
            // Get the meter index
            int meterIdx = Serial.parseInt();
            if (meterIdx < NUM_KEGS && meterIdx >= 0) {
              // There should be another part of the message, wait for it
              while (Serial.available() == 0);
              setKegType(kegMeters[meterIdx], Serial.read());
            }
            else {
              Serial.print("Command failed, no keg found with index ");
              Serial.println(meterIdx);
            }
          }
          else {
            Serial.println("Command failed, option not found.");
          }
          break;
        
        case FILL_LOAD_WINDOW_CHAR: {
          while (Serial.available() == 0);
          float serialReadKg = Serial.parseFloat();
          if (serialReadKg >= 0) {
             for (int i = 0; i < NUM_KEGS; i++) {
               kegLoadsInKg[i] = serialReadKg;
             }
             Serial.print("Keg load window now filled with value: ");
             Serial.println(serialReadKg);
          }
          else {
            Serial.println("Command failed, invalid kg value."); 
          }
          break;
        }
        default:
          Serial.println("No such command was found.");
          break;    
      }
    }

    while (Serial.available() > 0 && Serial.peek() != '|') { Serial.read(); }
  }
}

#define CORNY_KEG_CHAR 'c'
#define SANKE_50L_KEG_CHAR 's'

void setKegType(KegLoadMeter& kegMeter, char kegType) {
  switch (kegType) {
    
    case CORNY_KEG_CHAR:
      kegMeter.setKegType(KegLoadMeter::Corny);
      printKegTypeSetMsg(kegMeter, "Corny");
      break;
      
    case SANKE_50L_KEG_CHAR:
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

