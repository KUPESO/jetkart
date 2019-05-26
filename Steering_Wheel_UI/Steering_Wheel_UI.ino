/*
  ____  _____ ____   ___  
 |  _ \| ____/ ___| / _ \ 
 | |_) |  _| \___ \| | | |
 |  __/| |___ ___) | |_| |
 |_|   |_____|____/ \___/ 
 University of Kansas Physics and Engineering Student Organization
 -----------------------------------------------------------------
 Turboshaft Vehicle Steering Wheel Display
 -----------------------------------------------------------------
 Main Controller - ATMEGA2560
 -----------------------------------------------------------------
 ©2016-2017
 Elise McEllhiney
 Austin Feathers
 Preston Rabe
 -----------------------------------------------------------------
 
 */
#include <Adafruit_NeoPixel.h>

byte throttleval = 0;

#define LED_PIN     22
#define LED_COUNT   11
Adafruit_NeoPixel strip = Adafruit_NeoPixel(11, 22, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);    //FADEC
  delay(8000);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  int n = 1;
  if(Serial.available() > 0)
       {
          //throttleval = Serial.read();
          strip.setPixelColor(0, 0, 255, 0);
          strip.show();
          delay(500);
       }
  else
  {
     strip.setPixelColor(0, 0, 0, 255);
     strip.show();
  }
  n=1;
}
