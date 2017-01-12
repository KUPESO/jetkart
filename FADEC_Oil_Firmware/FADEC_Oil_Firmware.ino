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
Oil Pressure Regulation - ATMEGA328p
-----------------------------------------------------------------
©2016-2017
Austin Feathers
Jordan Sprick
Preston Rabe
Laurissa Marcotte
-----------------------------------------------------------------

*/

#define ledpin 9  //goes to LED
#define pwmpin 5  //goes to PWM controller
#define pressurein 14  //pressure sensor connected to analog pin 14
#define oilcheck 4  //pin that monitors if oil pressure is within an acceptable range

boolean oilok = LOW;  //indicates whether or not oil pressure is ok
int pressure = 0;  //oil pressure
byte psi = 0;
int pumppower = 150; //duty cycle supplied to oil pump
int upperbound = 52;
int lowerbound = 48;
int maxpressure = 60;
int minpressure = 40;

void setup()
{
 Serial.begin(9600);  //open serial port
 delay(500);
}

void loop()
{
  pressure = (analogRead(pressurein)/4);  //read pressure from port 14
  psi = (pressure-25)/2;
  Serial.write(psi);  //display value
  if ((psi > maxpressure) || (psi < minpressure))
  {
    oilok = LOW;  //indicate that oil pressure is outside acceptable range
    digitalWrite(oilcheck, LOW);
  }
  else
  {
     oilok = HIGH;  //indicate that oil pressure is within acceptable range
     digitalWrite(oilcheck, HIGH);
  }
  
  if (psi > upperbound)
  {
    pumppower--;  //decrease pumppower
  }
 else if (psi < lowerbound)
 {
    pumppower++;  //increase pumppower
 }

  digitalWrite(ledpin, oilok);
  digitalWrite(oilcheck, oilok);
  analogWrite(pwmpin, pumppower);  //operate oil pump at new duty cycle
  delay(100);  //allow oil pressure to adjust to new pump power
}
