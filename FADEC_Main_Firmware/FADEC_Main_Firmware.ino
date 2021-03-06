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
 ©2016-2019
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
#define throttlereadrate 20     //rate at which throttle is read at, used for slew rate function, CHANGE IF YOU CHANGE THROTTLEREADTIME (1000/throttlereadtime)

Servo STARTER;    //name servo

//initialize max6675 thermocouple A
#define AthermoDO 40
#define AthermoCS 39
#define AthermoCLK 38
MAX6675 Athermocouple(AthermoCLK, AthermoCS, AthermoDO);

//initialize max6675 thermocouple B
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

const int steeringtime = 100; //steering wheel data transmission interval
unsigned long laststeeringtime = 0; //time of last steering wheel data transmission

const int requesttime = 250; // start request polling interval
unsigned long lastrequesttime = 0; // start request polling counter

const int throttlereadtime = 50; // throttle read interval
unsigned long lastthrottlereadtime = 0; // throttle read counter 
int throttlediff = 0; // Instantaneous difference between throttle input and fuel speed

int idle = 90; // Default idle setting
const int idlesettime = 1000; // idle adjust interval - should be high, on the order of seconds, for stable idle
unsigned long lastidlesettime = 0; // idle adjust counter

unsigned long preheattimer = 0; // variable for timing preheat
unsigned long starttimer = 0; // variable for timing start sequence
unsigned long runtimer = 0; // variable for timing events while running
unsigned long cooltimer = 0; // variable for timing cooldown cycle

byte oilpsi = 0; //1 PSI precision to max of 100 PSI
long RPM = 0; //max ~80,000 RPM, precision = whatever necessary, currently 312 RPM per step
int throttlesetting = 0; //
int fuelspeed = 0;
int startspeed = 0;
int startswLED_val = 0;
byte slewrate = 0;
int groundspeed = 0;  //GPS speed

boolean FADECwhiteLED_val = LOW;
boolean FADECorangeLED_val = LOW;
boolean FADECgreenLED_val = LOW;

//TESTING ONLY
boolean NOchill = LOW;
//TESTING ONLY

//temperature initialization - unconsequential as of current version
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
  Serial1.begin(115200);
  Serial.begin(9600);
  delay(1000);
  Serial.print("CONNECTED 1.7\n");     //current version, update when you make changes
  
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
  steeringwheel();
  
  state = 0;  
//---------------------------------------------------------------------------------------------------------
  while // HEATING - this loop runs when a start requested. It runs for at least 30s, and until oil and combustor are hot.
    ((oilokay == HIGH) 
    && (requeststart == HIGH) 
    && (wantstart == LOW)  
    && (wantrun == LOW)  
    && (estop == HIGH))
  {
    if(state != 1)
      {
        preheattimer = millis();  //happens only on the first run of the loop
      }
    estop = digitalRead(estopsig_pin);
    oilokay = digitalRead(oilokay_pin);
    digitalWrite(startengsol_pin, HIGH);
    oilread();
    tachread();
    temp();
    telemetry();
    steeringwheel();
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
        idle = 90;    //Re-initialize idle value in case previous run idleset function set it too low (hot vs cold oil)
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
      if(state != 2)
      {
        //happens only on the first run of the loop
      }
      estop = digitalRead(estopsig_pin);
      oilokay = digitalRead(oilokay_pin);
      oilread();
      tachread();
      temp();
      telemetry();
      steeringwheel();
      state = 2;

    if(millis()-starttimer < 3000)
      {
        STARTER.write(45);
        analogWrite(fuelpwmout_pin, 60);
      }
    else if (millis()-starttimer >= 3000 && millis()-starttimer < 15000 && RPM < 15000)
      {
        digitalWrite(FADECorange_pin, HIGH);
        STARTER.write(30);
        analogWrite(fuelpwmout_pin, 75);
      }
    else if (millis()-starttimer < 15000 && RPM >= 15000 && RPM < 22000)
      {
        digitalWrite(FADECorange_pin, LOW);
        STARTER.write(25);
        analogWrite(fuelpwmout_pin, 85);
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
      && (RPM > 25000)
      && (RPM < 70000))
      {
      if(state != 3)
      {
        fuelspeed = 0; //happens only on the first run of the loop
      }
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
      steeringwheel();
      state = 3;  

     if(millis()-runtimer < 600000)
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
        
      if((temperatureegt > 1500) || (temperatureegt < 600))            // Overtemp/undertemp - Return to passive or cooldown
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
      if( 3 == state )//if we just exited the run state print why we exited
    {
      if(!(oilokay == HIGH))
        {
          Serial.print("Exit: oilokay LOW oilpsi:");
          Serial.print(oilpsi);
        }
      else if(!(requeststart == LOW))
        {
          Serial.print("Exit: requeststart HIGH");
        }
      else if(!(wantstart == LOW))
        {
          Serial.print("Exit: wantstart HIGH");
        }
      else if(!(wantrun == HIGH))
        {
          Serial.print("Exit: wantrun LOW");
        }
      else if(!(estop == HIGH))
        {
          Serial.print("Exit: estop LOW");
        }
      else if(!(RPM > 25000))
        {
          Serial.print("Exit: RPM: ");
          Serial.print(RPM);
          Serial.print(" <= 25000");
        }
      else if(!(RPM < 70000))
        {
          Serial.print("Exit: RPM: ");
          Serial.print(RPM);
          Serial.print(" >= 70000");
        }
      else
        {
          Serial.print("Exit: UNKNOWN! Check code for new exit conditions!");
        }
      Serial.println();
    }
//---------------------------------------------------------------------------------------------------------
    while // COOLDOWN
      ((oilokay == HIGH) && (requeststart == LOW) && (wantstart == LOW)  && (wantrun == LOW) && (estop == HIGH) && (hot == HIGH) && (NOchill == LOW))
      {
      if(state != 4)
      {
        cooltimer = millis();                   //Set cooltimer before entering cooldown to prevent instantaneous starter motor engagement
        fuelspeed = 0;
        throttlediff = 0;
        throttlesetting = 0;                    //Set throttle back to 0 upon entering cooldown to prevent a non-zero throttle from being applied on next start//happens only on the first run of the loop
      }
      estop = digitalRead(estopsig_pin);
      oilokay = digitalRead(oilokay_pin);
      request();
      oilread();
      tachread();
      temp();
      telemetry();
      steeringwheel();
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
    if((idle >= 95) || (idle <= 55))  //if idle setting unreasonably high or low, step it back to default
    {
      idle = 80;
    }
    else if(RPM < 30000)  //if idle is low, increment idle
    {
      idle++;
    }
    else if(RPM >= 30000)   //if idle is high, decrement idle
    { 
      idle--;
    }
    lastidlesettime = millis();
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
    if(oilokay == HIGH) // Set green LED according to "oil pressure okay/not okay"
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
  int analogval = 0;
  if(millis()-lastthrottlereadtime >= throttlereadtime)   //If it is time to read the throttle (50ms interval - THIS INTERVAL AFFECTS THROTTLE SLEW RATE LIMIT)
    {
      analogval = analogRead(throttlein_pin);             //read potentiometer value
      if(analogval < 495)                             //minimum trigger pull reads 0.9V (185), if below 185 set to minimum value
        {
          throttlesetting = 0;
        }
      else if(analogval > 870)                        //maximum trigger pull reads 4.26V (870), if above 870 set to maximum value
      {
        throttlesetting = 255;
      }
      else
      {
        throttlesetting = map(analogval,495,870,0,255);               //otherwise map 185-870 to 0-255 range
      }

      if(throttlesetting < 15)    //Prevent jitter-related bugs by defining thresholds for "high" and "low" potentiometer settings
        {
          throttlesetting = 0;
        }
      else if(throttlesetting > 245)
        {
          throttlesetting = 255;
        }
      
      throttlediff = throttlesetting - fuelspeed;    //Calculate difference between desired throttle setting and current fuel setting
              
      if(throttlediff > 0)   //If throttle setting is higher than fuel setting
        {
          if(RPM > 30000)
          {
            slewrate = ((180 + ((RPM - 30000)/333)) / throttlereadrate);   //168->180 //403->333  //calculate slewrate value by dividing RPM - 30000 by 403 and then adding to the base slew value of 168. All that then gets divided by the number of times per second the function is called at (throttlereadrate)
            if(throttlediff < slewrate)
            {
              fuelspeed += abs(throttlediff);                                //if throttlediff is below the max slewrate for that RPM, set fuelspeed to throttlesetting
            }
            else
            {
              fuelspeed += abs(slewrate);                                    //if throttlediff is greater than the max slewrate, slew by the max slewrate
            }
          }
          else                                                               //if RPM is below 30k, obey default slew speed 
          {
            if(throttlediff < (9))                                           //if difference is less than the base slew rate (168 / throttlereadrate)
            {
              fuelspeed += abs(throttlediff);                                //then slew by the difference
            }
            else
            {
              fuelspeed += (9);                                              //else slew by base slew rate (168 / throttlereadrate)
            }
          }
        }
      else if(throttlediff < 0)   //If throttle setting is lower than fuel setting
        {
          if(RPM > 30000)
          {
            slewrate = ((80 + ((RPM - 30000)/282)) / throttlereadrate);  //915->348 //348->282   //calculate slewrate value by dividing RPM - 30000 by 915 and then adding to the base slew value of 80. All that then gets divided by the number of times per second the function is called at (throttlereadrate)
            if(abs(throttlediff) < slewrate)
            {
              fuelspeed -= abs(throttlediff);                                //if throttlediff is below the max slewrate for that RPM, set fuelspeed to throttlesetting
            }
            else
            {
              fuelspeed -= abs(slewrate);                                    //if throttlediff is greater than the max slewrate, slew by the max slewrate
            }
          }
          else                                                               //if RPM is below 30k, obey default slew speed 
          {
            if(abs(throttlediff) < (4))                                      //if difference is less than the base slew rate (80 / throttlereadrate)
            {
              fuelspeed -= abs(throttlediff);                                //slew by difference
            }
            else
            {
              fuelspeed -= (4);                                              //else slew by base slew rate (80 / throttlereadrate)
            }
          }
        }
      lastthrottlereadtime = millis();    //Reset throttleread function counter
    }
  groundspeed = fuelspeed/3;  //dummy value for steering wheel GPS speed testing
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
          Serial.println("PASS");
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
      Serial.println(temperatureoil);
      Serial.println((int)oilpsi);
      Serial.println(temperatureegt);
      Serial.println(RPM);
      Serial.println(fuelspeed);
      Serial.println(groundspeed);
      Serial.println("#");
      digitalWrite(FADECwhite_pin, LOW);
      lasttelemetrytime = millis();  
  }
}
char outstr[53];
void steeringwheel()
{
  if((millis() - laststeeringtime) >= 200)//steeringtime)
  {
    snprintf(outstr, 53, "%d %03d %02d %04d %05ld %03d %03d %02d \n", state, temperatureoil, oilpsi, temperatureegt, RPM, throttlesetting, fuelspeed, groundspeed);
    /*Serial1.print(temperatureoil);
    Serial1.print(",");
    Serial1.print(oilpsi);
    Serial1.print(",");
    Serial1.print(temperatureegt);
    Serial1.print(",");
    Serial1.print(RPM);
    Serial1.print(",");
    Serial1.print(throttlesetting);
    Serial1.print(",");
    Serial1.print(fuelspeed);
    Serial1.println("");*/
    Serial1.print(outstr);
    laststeeringtime = millis();
  }
  
}
