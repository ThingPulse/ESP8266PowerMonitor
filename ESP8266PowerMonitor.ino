/**
The MIT License (MIT)
Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
Please note: We are spending a lot of time developing Software and Hardware
for these projects. Please consider supporting us by
a) Buying my hardware kits from https://thingpulse.com
b) Send a donation: https://paypal.me/thingpulse/5USD
c) Or using this affiliate link while shopping: https://www.banggood.com/?p=CJ091019038450201802
See more at https://thingpulse.com

*/

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"`
#include "Adafruit_INA219.h"

/** Config **/
// If a current in mA above this value is measured the device is awake
#define AWAKE_THRESHOLD 1.0

// The battery used for forecasting the runtime in days in mAh
#define BAT_CAPACITY 600
/** End Config */


// States of the monitored device
#define STATE_NA 0
#define STATE_SLEEPING 1
#define STATE_AWAKE 2

// initial state
int deviceState = STATE_NA;

// Create OLED and INA219 globals.
SSD1306  display(0x3c, D5, D6);
Adafruit_INA219 ina219;

uint32_t lastUpdateMillis = 0;
uint32_t lastScreenUpdateMillis = 0;
uint32_t totalMeasuredMillis = 0;
uint32_t awakeStartMillis = 0;
uint32_t sleepStartMillis = 0;
uint32_t totalAwakeMillis = 0;
uint32_t totalSleepMillis = 0;
float totalAwakeMAh = 0;
float totalSleepMAh = 0;
float forecastBat = 0;

String getStateText(int state) {
  switch(state) {
    case STATE_NA:
      return "NA";
    case STATE_AWAKE:
      return "AWAKE";
    case STATE_SLEEPING:
      return "SLEEP";
  }
  return "?";
}

void setup()   {
  Serial.begin(115200);

  Serial.println("Setting up display...");
  // Setup the OLED display.
  display.init();

  deviceState = STATE_NA;

  // Clear the display.
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.display();

  ina219.begin(D3, D4);
  // By default the INA219 will be calibrated with a range of 32V, 2A.
  // However uncomment one of the below to change the range.  A smaller
  // range can't measure as large of values but will measure with slightly
  // better precision.
  //ina219.setCalibration_32V_1A();
  ina219.setCalibration_16V_400mA();

}

void loop() {
  // Read voltage and current from INA219.
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float currentMA = ina219.getCurrent_mA();


  uint32_t timeSinceLastMeasurement = millis() - lastUpdateMillis;
  totalMeasuredMillis += timeSinceLastMeasurement;
  uint32_t totalSec = totalMeasuredMillis / 1000;
  
  if (currentMA > AWAKE_THRESHOLD) {
    if (deviceState == STATE_NA) {
      deviceState = STATE_AWAKE;
      awakeStartMillis = millis();
      totalAwakeMAh = 0;
      totalMeasuredMillis = 0;
    } else if (deviceState == STATE_SLEEPING) {
      deviceState = STATE_AWAKE;
      awakeStartMillis = millis();
      totalAwakeMAh = 0;
      totalMeasuredMillis = 0;
    } else if (deviceState == STATE_AWAKE) {
      totalAwakeMillis = millis() - awakeStartMillis;
      totalAwakeMAh = totalAwakeMAh + ((currentMA * timeSinceLastMeasurement) / (3600 * 1000));
    }
  } else if (currentMA <= AWAKE_THRESHOLD) {
    if (deviceState == STATE_NA) {
      //deviceState = STATE_SLEEPING;
    } else if (deviceState == STATE_SLEEPING) {
      totalSleepMillis = millis() - sleepStartMillis;
      totalSleepMAh = totalSleepMAh + ((currentMA * timeSinceLastMeasurement) / (3600 * 1000));
      float forecastSleepMAh = 20 * 60 * 1000 * totalSleepMAh / totalSleepMillis;
      forecastBat = (((totalAwakeMillis + 20 * 60 * 1000) / 1000) / (144 * (totalAwakeMAh + forecastSleepMAh)));
    } else if (deviceState == STATE_AWAKE) {
      deviceState = STATE_SLEEPING;
      sleepStartMillis = millis();
      totalSleepMAh = 0;
    }
  }

  lastUpdateMillis = millis();
  if (millis() - lastScreenUpdateMillis > 500) {

    
    Wire.begin(D5, D6);
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(0, 0, "Awake:");
    display.drawString(0, 54, "State:");
    display.drawString(0, 20, "Sleep:");
    display.drawString(0, 40, "Forecast 0.6Ah:");
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(128, 0, String(totalAwakeMillis) + "ms");
    display.drawString(128, 10, String(totalAwakeMAh) + "mAh");
    display.drawString(128, 20, String(totalSleepMillis) + "ms");
    display.drawString(128, 30, String(totalSleepMAh, 6) + "mAh");
    display.drawString(128, 40, String(forecastBat) + "d");
    display.drawString(128, 54, getStateText(deviceState));
    display.display();
    Wire.begin(D3, D4);
    lastScreenUpdateMillis = millis();
  }

}
