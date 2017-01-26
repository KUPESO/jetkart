/*
 ____  _____ ____   ___  
|  _ \| ____/ ___| / _ \ 
| |_) |  _| \___ \| | | |
|  __/| |___ ___) | |_| |
|_|   |_____|____/ \___/ 
University of Kansas Physics and Engineering Student Organization
-----------------------------------------------------------------
Gas Turbine Full Authority Digital Engine Controller
-----------------------------------------------------------------
Tachometer - ATMEGA328p
-----------------------------------------------------------------
©2016
Austin Feathers
Jordan Sprick
Zach Brachtenbach
Preston Rabe
-----------------------------------------------------------------

*/

#define ledpin 5
#define tachpin 1

volatile byte revolutions;
volatile boolean led;
unsigned int rpm;
unsigned long timeold;

void rev_int()
{
  revolutions++;                                    //interrupt increments revolutions for every rotation
}

void setup()
{
  revolutions = 0;
  rpm = 0;
  timeold = 0;
  pinMode(ledpin, OUTPUT);
  digitalWrite(ledpin, LOW);
  
  Serial.begin(9600);
  attachInterrupt(tachpin, rev_int, FALLING);  
}

void loop()
{
  delay(100);                                        //update approximately every 100ms, interrupt active for 100ms
  detachInterrupt(tachpin);                                //stop interrupt
  
  rpm = (60000/(millis() - timeold))*revolutions;  //calculate millisecond-revolution value, calculate rpm
  //Serial.write(rpm/312);
  Serial.write(rpm/312);
//    Serial.write(357 / 256);
//    Serial.write(357 % 256);
  revolutions = 0;    //reset interrupt counter
  
      if(led == HIGH)
  {
    led = LOW;
    digitalWrite(ledpin, LOW);
  }
  else
  {
    led = HIGH;
    digitalWrite(ledpin, HIGH);
  }
  
  timeold = millis();                                //after other loop functions occur, update timeold (millis expires in 50 days, carefull)
  attachInterrupt(tachpin, rev_int, FALLING);             //restart interrupt
}
