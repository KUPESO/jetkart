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
 Main Controller - ATMEGA2560
 -----------------------------------------------------------------
 ©2016-2017
 Austin Feathers
 Jordan Sprick
 Preston Rabe
 Laurissa Marcotte
 -----------------------------------------------------------------
 
 */

#include <max6675.h>
#include <Servo.h>

#define estopsig_pin 2
#define fuelpwmout_pin 6
#define FADECwhite_pin 7
#define FADECorange_pin 8
#define FADECgreen_pin 9
#define escsigout_pin 12
#define startswled_pin 13
#define startswsig_pin 37
#define oilokay_pin 36
#define ignctl_pin 26
#define startfuelsol_pin 25
#define startengsol_pin 24
#define genrlyctl_1_pin 23
#define genrlyctl_2_pin 22
#define throttlein_pin 0

Servo STARTER;

#define AthermoDO 40
#define AthermoCS 39
#define AthermoCLK 38
MAX6675 Athermocouple(AthermoCLK, AthermoCS, AthermoDO);

#define BthermoDO 43
#define BthermoCS 42
#define BthermoCLK 41
MAX6675 Bthermocouple(BthermoCLK, BthermoCS, BthermoDO);

boolean oilokay = LOW;
boolean requeststart = LOW;
boolean wantstart = LOW;
boolean wantrun = LOW;
boolean oiltemp = LOW;
boolean estop = LOW;
boolean hot = LOW;

boolean ignstate = LOW;
const int ignhigh = 5; // duration of ignition dwell
const int ignlow = 250; // ignition low period
unsigned long lastigntime = 0; // ignition counter

const int temptime = 250; // temperature sample interval
unsigned long lasttemptime = 0;  // temperature sample counter

const int oilreadtime = 50; // oil pressure sample interval
unsigned long lastoilreadtime = 0; // oil pressure sample counter

const int tachreadtime = 50; // tachometer sample interval
unsigned long lasttachreadtime = 0; // tachometer sample counter

const int telemetrytime = 250; // telemetry transmission interval
unsigned long lasttelemetrytime = 0; // telemetry transmission counter

const int requesttime = 250; // start request polling interval
unsigned long lastrequesttime = 0; // start request polling counter

const int throttlereadtime = 50; // throttle read interval
unsigned long lastthrottlereadtime = 0; // throttle read counter 

int idle = 90; // Default idle setting
const int idlesettime = 1000; // idle adjust interval - should be high, on the order of seconds, for stable idle
unsigned long lastidlesettime = 0; // idle adjust counter

unsigned long preheattimer = 0; // variable for timing preheat
unsigned long starttimer = 0; // variable for timing start sequence
unsigned long runtimer = 0; // variable for timing events while running
unsigned long cooltimer = 0; // variable for timing cooldown cycle

byte oilpsi = 0;
long RPM = 0;
int fuelspeed = 0;
int startspeed = 0;
int startswLED_val = 0;

boolean FADECwhiteLED_val = LOW;
boolean FADECorangeLED_val = LOW;
boolean FADECgreenLED_val = LOW;

//TESTING ONLY
boolean NOchill = LOW;
//TESTING ONLY

int temperatureoil = 0;
int temperatureegt = 0;
int ambienttempoil = 0;
int ambienttempegt = 0;

byte state = 0;

void setup() 
{
  pinMode(FADECwhite_pin,OUTPUT);
  pinMode(FADECorange_pin,OUTPUT);
  pinMode(FADECgreen_pin,OUTPUT);
  pinMode(escsigout_pin, OUTPUT);
  pinMode(startswled_pin, OUTPUT);
  pinMode(startswsig_pin, INPUT);
  pinMode(oilokay_pin, INPUT);
  pinMode(ignctl_pin, OUTPUT);
  pinMode(startfuelsol_pin, OUTPUT);
  pinMode(startengsol_pin, OUTPUT);
  pinMode(genrlyctl_1_pin, OUTPUT);
  pinMode(genrlyctl_2_pin, OUTPUT);
  pinMode(throttlein_pin, INPUT);
  pinMode(fuelpwmout_pin, OUTPUT);
  digitalWrite(FADECgreen_pin,LOW);
  
  STARTER.attach(escsigout_pin);
  STARTER.write(85);
  
  Serial3.begin(9600);
  Serial2.begin(9600);
  Serial1.begin(9600);
  Serial.begin(9600);
  delay(500);
  Serial.print("CONNECTED\n");
  
  ambienttempoil = int(Athermocouple.readFarenheit());
  ambienttempegt = int(Bthermocouple.readFarenheit());
  if((ambienttempoil || ambienttempegt) > 110)
    {
      ambienttempoil = 80;
      ambienttempegt = 80;
    }

  delay(5000);
}

void loop() 
{
  //------------------------------------------------------------------- Default startup state - wait for oil pressure
  estop = digitalRead(estopsig_pin); // check emergency stop status
  oilokay = digitalRead(oilokay_pin); // check oil status
  wantstart = LOW;
  requeststart = LOW;
  wantrun = LOW;
  digitalWrite(startengsol_pin, LOW);
  digitalWrite(startfuelsol_pin, LOW);
  analogWrite(fuelpwmout_pin, 0);
  STARTER.write(84);

  request();
  oilread();
  tachread();
  temp();
  telemetry();
  throttleread();
  
  state = 0;
  preheattimer = millis();
  
//---------------------------------------------------------------------------------------------------------
  while // HEATING
    ((oilokay == HIGH) 
    && (requeststart == HIGH) 
    && (wantstart == LOW)  
    && (wantrun == LOW)  
    && (estop == HIGH))
  {
    estop = digitalRead(estopsig_pin);
    oilokay = digitalRead(oilokay_pin);
    digitalWrite(startengsol_pin, HIGH);
    oilread();
    tachread();
    temp();
    telemetry();
    state = 1;
    analogWrite(fuelpwmout_pin, 0);

    if(temperatureegt < 800)                       // Run ignition for low EGT
      {
        ignition();
      }
      
    if(millis()-preheattimer < 300)
      {
        STARTER.write(82);
      }
    else if(millis()-preheattimer >= 300 && millis()-preheattimer < 5000)
      {
        STARTER.write(75);
      }
    else if(millis()-preheattimer >= 5000 && millis()-preheattimer < 300000)
      {
        digitalWrite(startfuelsol_pin, HIGH);
      }
    else                                            // Timeout - Return to idle
      {
        wantstart = LOW;
        requeststart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(84);
      }
      
    if(temperatureegt > 1400)                // Overtemp - Return to idle
      {
        wantstart = LOW;
        requeststart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(84);
      }
      
    if((temperatureegt > 750) && (oiltemp == HIGH) && ((millis()-preheattimer) > 30000)) // Proceed to start
      {
        requeststart = LOW;
        wantstart =  HIGH;
        wantrun = LOW;
        starttimer = millis();
      }
    request();
  }
//---------------------------------------------------------------------------------------------------------
  while // STARTING
    ((oilokay == HIGH) 
    && (requeststart == LOW) 
    && (wantstart == HIGH)  
    && (wantrun == LOW) 
    && (estop == HIGH))
    {
      estop = digitalRead(estopsig_pin);
      oilokay = digitalRead(oilokay_pin);
      oilread();
      tachread();
      temp();
      telemetry();
      state = 2;

    if(millis()-starttimer < 5000)
      {
        STARTER.write(55);
        analogWrite(fuelpwmout_pin, 55);
      }
    else if (millis()-starttimer >= 5000 && millis()-starttimer < 15000 && RPM < 15000)
      {
        digitalWrite(FADECorange_pin, HIGH);
        STARTER.write(35);
        analogWrite(fuelpwmout_pin, 75);
      }
    else if (millis()-starttimer < 15000 && RPM >= 15000 && RPM < 22000)
      {
        digitalWrite(FADECorange_pin, LOW);
        STARTER.write(25);
        analogWrite(fuelpwmout_pin, 90);
      }
    else if (millis()-starttimer < 20000 && RPM >= 22000 && RPM < 30000)  
      {
        digitalWrite(FADECorange_pin, HIGH);
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(85);
        analogWrite(fuelpwmout_pin, 95);        
      }
    else if (millis()-starttimer < 20000 && RPM >= 30000)         // Started - Proceed to run
      {
        digitalWrite(FADECorange_pin, LOW);
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(85);
        analogWrite(fuelpwmout_pin, 100);
        wantstart = LOW;
        requeststart = LOW;
        wantrun = HIGH;
        runtimer = millis();
      }
    else                                                            // Timeout - Return to idle or cooldown
      {
        wantstart = LOW;
        requeststart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(84);
      }

    if((temperatureegt > 1500) || (temperatureegt < 500))            // Overtemp - Return to idle or cooldown
      {
        wantstart = LOW;
        requeststart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        STARTER.write(84);
      }
    request();
    }
//---------------------------------------------------------------------------------------------------------
    while // RUNNING
      ((oilokay == HIGH) 
      && (requeststart == LOW) 
      && (wantstart == LOW)  
      && (wantrun == HIGH) 
      && (estop == HIGH)
      && (RPM > 25000))
      {
      estop = digitalRead(estopsig_pin);
      oilokay = digitalRead(oilokay_pin);
      digitalWrite(startengsol_pin, LOW);
      digitalWrite(startfuelsol_pin, LOW);
      STARTER.write(85);
      throttleread();
      oilread();
      tachread();
      temp();
      telemetry();
      state = 3;  

     if(millis()-runtimer < 300000)
        {
          if(fuelspeed < 20)
            {
              idleset();
              analogWrite(fuelpwmout_pin, idle);
            }
          else if(fuelspeed >= 20)
            {
              analogWrite(fuelpwmout_pin, map(fuelspeed, 20, 255, idle, 255));
            }
        }
     else
        {
        wantstart = LOW;
        requeststart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        digitalWrite(startfuelsol_pin, LOW);
        analogWrite(fuelpwmout_pin, 0);
        STARTER.write(84);
        }
        
      if((temperatureegt > 1500) || (temperatureegt < 600))            // Overtemp/undertemp - Return to idle or cooldown
        {
          wantstart = LOW;
          requeststart = LOW;
          wantrun = LOW;
          cooltimer = millis();
          digitalWrite(startengsol_pin, LOW);
          digitalWrite(startfuelsol_pin, LOW);
          analogWrite(fuelpwmout_pin, 0);
          STARTER.write(84);
        }
      request();
      }
//---------------------------------------------------------------------------------------------------------
    while // COOLDOWN
      ((oilokay == HIGH) && (requeststart == LOW) && (wantstart == LOW)  && (wantrun == LOW) && (estop == HIGH) && (hot == HIGH) && (NOchill == LOW))
      {
      estop = digitalRead(estopsig_pin);
      oilokay = digitalRead(oilokay_pin);
      request();
      oilread();
      tachread();
      temp();
      telemetry();
      analogWrite(fuelpwmout_pin, 0);
      digitalWrite(startfuelsol_pin, LOW);
      state = 4;

      if(RPM < 2000)
        {
          if(millis()-cooltimer > 10000 && millis()-cooltimer < 10300)
            {
              digitalWrite(startengsol_pin, HIGH);
              STARTER.write(82);
            }
          else if(millis()-cooltimer >= 10300 && millis()-cooltimer < 300000)
            {
              digitalWrite(startengsol_pin, HIGH);
              STARTER.write(75);
            }          
        }
      }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void request()
{
  if(millis() - lastrequesttime >= requesttime)
  {
    if( Serial.available()>0)
     {
       if(Serial.readString() == "start")
       {
       requeststart = HIGH;
       }
       else
       {
        requeststart = LOW;
        wantstart = LOW;
        wantrun = LOW;
        cooltimer = millis();
        digitalWrite(startengsol_pin, LOW);
        digitalWrite(startfuelsol_pin, LOW);
        analogWrite(fuelpwmout_pin, 0);
        digitalWrite(ignctl_pin, LOW);
        STARTER.write(84);
       }
       Serial.readString();
       Serial.readString();
     }
     lastrequesttime = millis();
  }
}

void idleset() //this function maintains idle speed at ~30,000 RPM. This accommodates for oil temperature fluctuations and different fuels.
{
  if(((millis() - lastidlesettime) >= idlesettime) && (fuelspeed < 20)) //adjust idle speed only when in idle state and at the specified interval
  {
    if((idle >= 105) || (idle <= 75))  //if idle setting unreasonably high or low, step it back to default
    {
      idle = 90;
    }
    if(RPM < 30000)  //if idle is low, increment idle
    {
      idle++;
    }
    else if(RPM > 30500)   //if idle is high, decrement idle
    { 
      idle--;
    }
  }
}

void ignition()
{
  if(ignstate == LOW)
  {
    if(millis() - lastigntime >= ignlow)
    {
      digitalWrite(ignctl_pin, HIGH);
      ignstate = HIGH;
      lastigntime = millis();
    }
  }
  else
  {
    if(millis() - lastigntime >= ignhigh)
    {
      digitalWrite(ignctl_pin, LOW);
      ignstate = LOW;
      lastigntime = millis();
    }
  }
}


void temp()
{
  if(millis()- lasttemptime >= temptime)
  {
    temperatureoil = Athermocouple.readFarenheit();
    temperatureegt = Bthermocouple.readFarenheit();
    if(temperatureoil > 100)
      {
        oiltemp = HIGH;
      }
    else if(temperatureoil < 95)
      {
        oiltemp = LOW;
      }
    if(temperatureegt > 350)
      {
 //       digitalWrite(FADECorange_pin, HIGH);
        hot = HIGH;
      }
    else if(temperatureegt < 250)
      {
 //       digitalWrite(FADECorange_pin, LOW);
        hot = LOW;
      }
    lasttemptime = millis();
  }
}


void oilread()
{
  if(millis() - lastoilreadtime >= oilreadtime)
  {
    if( Serial3.available() > 0 ) // read oil pressure
    {
      oilpsi = Serial3.read();
    }
    lastoilreadtime = millis();
  }
    if(oilokay == HIGH)
  {
    digitalWrite(FADECgreen_pin, HIGH);
  }
  else
  {
    digitalWrite(FADECgreen_pin, LOW);
  }
}

void throttleread()
{
  if(millis()-lastthrottlereadtime >= throttlereadtime)
    {
      if(analogRead(throttlein_pin)< 15)
        {
          fuelspeed = 0;
        }
      else
        {
          fuelspeed = (analogRead(throttlein_pin))/4;
        }
    }
}

void tachread()
{
  if(millis() - lasttachreadtime >= tachreadtime)
  {
    if( Serial2.available()>0)
     {
       RPM = (Serial2.read());  //    RPM = (int)RPM1*256+(int)RPM2;
       RPM = (RPM*312);
     }
     lasttachreadtime = millis();
  }
}

void telemetry()
{
  if(millis() - lasttelemetrytime >= telemetrytime)
  {
      digitalWrite(FADECwhite_pin, HIGH);
      switch(state)
      {
        case 0:
          Serial.println("IDLE");
          break;
        case 1:
          Serial.println("HEAT");
          break;
        case 2:
          Serial.println("START");
          break;
        case 3:
          Serial.println("RUN");
          break;
        case 4:
          Serial.println("COOL");
          break;
      }
      Serial.print("OT ");
      Serial.println(temperatureoil);
      Serial.print("OP ");
      Serial.println((int)oilpsi);
      Serial.print("EGT ");
      Serial.println(temperatureegt);
      Serial.print("RPM ");
      Serial.println(RPM);
      Serial.print("THR ");
      Serial.println(fuelspeed);
      Serial.println();
      Serial1.write((byte)RPM);
      digitalWrite(FADECwhite_pin, LOW);
      lasttelemetrytime = millis();  
  }
}

