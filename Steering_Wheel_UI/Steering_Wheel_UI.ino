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
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1325.h>

#define LED_PIN     22
#define LED_COUNT   12
Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, 22, NEO_GRB + NEO_KHZ800);

// If using software SPI, define CLK and MOSI
#define OLED_CLK 13
#define OLED_MOSI 11

// These are neede for both hardware & softare SPI
#define OLED_CS 10
#define OLED_RESET 9
#define OLED_DC 8

// this is software SPI, slower but any pins
Adafruit_SSD1325 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// this is for hardware SPI, fast! but fixed oubs
//Adafruit_SSD1325 display(OLED_DC, OLED_RESET, OLED_CS);

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
  
// The Arduino UNO doesnt have enough RAM for gradients
// but the *display* supports it!
void graydient()
{
  unsigned char x,y;
  display.command(0x15); /* set column address */
  display.command(0x00); /* set column start address */
  display.command(0x3f); /* set column end address */
  display.command(0x75); /* set row address */
  display.command(0x00); /* set row start address */
  display.command(0x3f); /* set row end address */
  for(y=0;y<64;y++) {
    for(x=0;x<4;x++) {
  display.data(0x00);
  display.data(0x11);
  display.data(0x22);
  display.data(0x33);
  display.data(0x44);
  display.data(0x55);
  display.data(0x66);
  display.data(0x77);
  display.data(0x88);
  display.data(0x99);
  display.data(0xAA);
  display.data(0xBB);
  display.data(0xCC);
  display.data(0xDD);
  display.data(0xEE);
  display.data(0xFF);
    }
  }
}

float safe_map(float value, float fromLow, float fromHigh, float toLow, float toHigh)
{
  if(value >= fromHigh)
  {
    return (toHigh);
  }
  else if(value <= toLow)
  {
    return (toLow);
  }
  else
  {
    return(((value - fromLow)/((fromHigh - fromLow) / (toHigh - toLow))) + toLow);
  }
  
}

void draw_background()
{
  display.drawRect(2,4,45,8,WHITE); //top left bargraph border
  display.drawRect(2,13,21,11,WHITE); //top left digits border
  display.drawRect(81,4,45,8,WHITE); //top right bargraph border
  display.drawRect(99,13,27,11,WHITE);//top right digits border
  display.drawRect(49,1,30,11,WHITE); //top center state box
  display.drawRect(55,14,18,13,WHITE);//middle center gps speed
  
  display.drawRect(3,33,122,14,WHITE);//middle center throttle border

  display.drawRect(3,33,122,14,WHITE);//middle center throttle border

  display.drawLine(0,54,127,54,WHITE);//bottom line
}

uint8_t state;

char pass_str[5] = "PASS";
char heat_str[5] = "HEAT";
char strt_str[5] = "STRT";
char  run_str[5] = "RUN!";
char cool_str[5] = "COOL";

void draw_state( uint8_t state )
{
  char *state_str_ptr;
  display.setCursor(51,3);
  switch( state ){
    case 0:
      state_str_ptr = pass_str;
    break;
    case 1:
      state_str_ptr = heat_str;
    break;
    case 2:
      state_str_ptr = strt_str;
    break;
    case 3:
      state_str_ptr = run_str;
    break;
    case 4:
      state_str_ptr = cool_str;
    break;
    default:
      display.print(state);
    return;
  }

  display.print(state_str_ptr[0]);
  display.setCursor(58,3);
  display.print(state_str_ptr[1]);
  display.setCursor(65,3);
  display.print(state_str_ptr[2]);
  display.setCursor(72,3);
  display.print(state_str_ptr[3]);
 
}

void updateLED( long RPM )
{
  //set all leds to 0
  strip.clear();;
  
  if(RPM > 10000)
  {
    strip.setPixelColor(0, 0, 0, 255);
    strip.setPixelColor(1, 0, 0, 255); 
  }
  if(RPM > 20000)
  {
    strip.setPixelColor(1, 0, 0, 255);
  }
  if(RPM > 30000)
  {
    strip.setPixelColor(2, 0, 255, 0);
  }
  if(RPM > 35000)
  {
    strip.setPixelColor(3, 0, 255, 0);
  }
  if(RPM > 40000)
  {
    strip.setPixelColor(4, 0, 255, 0);
  }
  if(RPM > 45000)
  {
    strip.setPixelColor(5, 0, 255, 0);
  }
  if(RPM > 50000)
  {
    strip.setPixelColor(6, 0, 255, 0);
  }
  if(RPM > 55000)
  {
    strip.setPixelColor(7, 0, 255, 0);
  }
  if(RPM > 60000)
  {
    strip.setPixelColor(8, 0, 255, 0);
  }
  if(RPM > 61000)
  {
    strip.setPixelColor(9, 255, 0, 0);
  }
  if(RPM > 62000)
  {
    strip.setPixelColor(10, 255, 0, 0);
  }
  if(RPM > 63000)
  {
    strip.setPixelColor(11, 255, 0, 0);
  }
  if(RPM > 63960)
  {
    
  }
  strip.show();
}

String instr;
int bytes_avail = 0;

char input_string[50];

char bottom_string[50];

char *working_string;

char *state_str;
char *oiltemp_str;
char *oilpsi_str;
char *egttemp_str;
char *rpm_str;
char *throttle_set_str;
char *fuelspeed_str;
char *gndspeed_str;
int16_t oil_temp;
uint8_t oil_temp_bug;
int16_t egt_temp;
uint8_t egt_bar_width;
uint8_t egt_bug;
int16_t throttle;
uint8_t throttle_bug;
uint8_t throttle_width;
int16_t fuel_pwm;
long RPM;
int buttonPin = A10;

char *ptr;

void setup()   {                
  Serial.begin(115200);
  //Serial.println("SSD1325 OLED test");

  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin();
  display.setRotation(2);
  display.clearDisplay();
  //display.display();

  draw_background();
  display.display();
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  int i = 0;
  RPM = 1;
  for (i = 0; i < 15; i++ )
  {
    RPM += 5000;
    updateLED(RPM);
    delay(50);
  }
  delay(1500);
  RPM = 0;
  updateLED(RPM);
  pinmode(buttonPin, INPUT);
  // init done
}

void loop() {
  display.setTextSize(1);
  display.setTextColor(WHITE,BLACK);
  bytes_avail = Serial.available();

  buttonState = digitalRead(buttonPin);
  if(buttonState == HIGH)
  {
    write...
  }
  
  if(bytes_avail > 0) 
  {

    instr = Serial.readStringUntil('\n');
    while(Serial.available())
      instr = Serial.readStringUntil('\n');
    display.clearDisplay();
    draw_background();
    
    memcpy(input_string,instr.c_str(),instr.length() < 49? instr.length(): 49);
    //display.clearDisplay();

    state_str = strtok(input_string, " ");
    oiltemp_str = strtok(0, " ");
    oilpsi_str = strtok(0, " ");
    egttemp_str = strtok(0, " ");
    rpm_str = strtok(0, " ");
    throttle_set_str = strtok(0, " ");
    fuelspeed_str = strtok(0, " \n");
    gndspeed_str = strtok(0, " \n");

    display.setCursor(1,56);
    snprintf(bottom_string, 50, "%s %s %s %s",throttle_set_str,fuelspeed_str,rpm_str,oilpsi_str);
    display.println(bottom_string);
    
    //Print state
    state = (uint8_t)strtol(state_str,&ptr, 10 );
    draw_state(state);
    
    //Print oil temp
    display.setCursor(4,15);
    display.print(oiltemp_str);
    oil_temp = (int16_t)strtol(oiltemp_str,&ptr, 10 );
    
    display.fillRect(4,6,41,4,BLACK);
    display.fillRect(4,6,(oil_temp/6),4,WHITE);
    display.fillRect(3,1,43,2,BLACK);
    oil_temp_bug = 100/6;
    display.drawLine(oil_temp_bug+4,2,oil_temp_bug+4+1,1,WHITE);
    display.drawLine(oil_temp_bug+4,2,oil_temp_bug+4-1,1,WHITE);
    
    //Print egt
    display.setCursor(101,15);
    display.print(egttemp_str);
    egt_temp = (int16_t)strtol(egttemp_str,&ptr, 10 );

    switch(state)
    {
      case 0:
      case 1:
        egt_bar_width = egt_temp / 20;
        egt_bug = 750/20;
        break;   
      case 2:
      case 3:
        egt_bar_width = (egt_temp - 580) / 20;
        egt_bug = 1000/20;
        break;
      case 4:
        egt_bar_width = egt_temp / 20;
        egt_bug = 250/20;
        break;   
      default:
        egt_bar_width = egt_temp / 20;
        egt_bug = 750/20;
        break; 
    }

    display.fillRect(83,6,41,4,BLACK);
    display.fillRect(83,6,egt_bar_width,4,WHITE);
    display.fillRect(82,1,43,2,BLACK);
    
    display.drawLine(egt_bug+83,2,egt_bug+83+1,1,WHITE);
    display.drawLine(egt_bug+83,2,egt_bug+83-1,1,WHITE);

    //Print throttle gauge
    throttle = (int16_t)strtol(throttle_set_str,&ptr, 10 );
    fuel_pwm = (int16_t)strtol(fuelspeed_str,&ptr, 10 );
    
    
    
    throttle_width = (uint8_t)safe_map(fuel_pwm, 0,255,0,118);
    display.fillRect(5,35,118,10,BLACK);
    display.fillRect(5,35,throttle_width,10,WHITE);
    display.fillRect(2,28,124,4,BLACK);
    display.fillRect(2,48,124,4,BLACK);

    throttle_bug = (uint8_t)safe_map(throttle, 0,255,0,117);
    display.drawLine(throttle_bug+5,31,throttle_bug+5+3,28,WHITE);
    display.drawLine(throttle_bug+5,31,throttle_bug+5-3,28,WHITE);
    display.drawLine(throttle_bug+5,48,throttle_bug+5+3,51,WHITE);
    display.drawLine(throttle_bug+5,48,throttle_bug+5-3,51,WHITE);
    

    
    //Print gps speed
    display.setCursor(58,17);
    display.print("8");
    display.setCursor(65,17);
    display.print("8");


    //debug lines:
    /*
    display.fillRect(28,18,19,9, BLACK);
    display.setCursor(28,18);
    display.println(oiltemp_str);
    */
    
    display.display();
    RPM = strtol(rpm_str,&ptr, 10 );
    updateLED(RPM);
    
  }
}
