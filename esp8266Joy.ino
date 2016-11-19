

/*
 * Joystick for RC control using ESP8266 and Blynk
 */

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <EEPROM.h>


// You should get YOUR OWN Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "1111141e130c455551dd98f7bdcd0000";

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "yourwifissid";
char pass[] = "yourwifipassword";

Servo s1, s2;  // Define two servo outputs


#define XServo 4
#define YServo 5

int curr1, curr2;  // current positions

// Will store these in EEPROM so they are sticky
int off1=0, off2=0;   // offsets for center
int raxis=0;          // reverse axes
int s1flip=0;         // flip servo 1
int s2flip=0;         // flip servo 2

#define EESIZE (sizeof(int)*6)  // remember, you need one extra int for the signature
const int EESIG=0xA1A1;   // if this isn't in EEPROM our data isn't either

// Because the EEPROM code crashes on some writes I am going to 
// make you have to save explicitly
int eedirty=0;  // EEPROM needs writing


// These update the servos so they can be called
// from several places
void updates1()
{
  int v=curr1+off1;   // process offset
  if (s1flip) v=3000-v;   // and flip
  // write to the correct servo based on raxis
  if (!raxis) s1.writeMicroseconds(v); else s2.writeMicroseconds(v);
}

void updates2()
{
  int v=curr2+off2;  // process offset
  if (s2flip) v=3000-v;  // and flip
  // write to the correct servo based on raxis
  if (!raxis) s2.writeMicroseconds(v); else s1.writeMicroseconds(v);
}

// Update both servos
void updateservos()
{
  updates1();
  updates2();
}


void setup()
{
  int addr=0;   // eeprom address
  int tmp;
  Serial.begin(9600);   // start up serial port 
// Set servos to default channel regardless of EEPROM setting
  s1.attach(XServo);         // Turn on servos
  s2.attach(YServo);
  EEPROM.begin(EESIZE);    // ESP8266 emulates EEPROM so start it up
  EEPROM.get(addr,tmp);    // see if our data is there
  if (tmp==EESIG) // make sure it looks like our data is in EEPROM
  {
    addr+=sizeof(int);   // Load data
    EEPROM.get(addr,raxis);
    addr+=sizeof(int);
    EEPROM.get(addr,off1);
    addr+=sizeof(int);
    EEPROM.get(addr,off2);
    addr+=sizeof(int);
    EEPROM.get(addr,s1flip);
    addr+=sizeof(int);
    EEPROM.get(addr,s2flip);
  }
  else eedirty=1;  // no eeprom data so mark that EEPROM needs saving
  // Start up Blynk
  Blynk.begin(auth, ssid, pass);  // ans WiFi
  // Start in the center
  curr1=curr2=1500;
  updateservos();  // Update them
}

// Save EEPROM values if eedirty is true
void _updateEE()  // This seems to occasionally hang the system
{ 
  int addr=0;
    if (!eedirty) return;
    EEPROM.put(addr,EESIG);
    addr+=sizeof(int);
    EEPROM.put(addr,raxis);
    addr+=sizeof(int);
    EEPROM.put(addr,off1);
    addr+=sizeof(int);
    EEPROM.put(addr,off2);  
    addr+=sizeof(int);
    EEPROM.put(addr,s1flip);  
    addr+=sizeof(int);
    EEPROM.put(addr,s2flip);  
    yield();   // doesn't seem to help hangs
    EEPROM.commit();   // write to EEPROM
    yield();
    eedirty=0;
}

// Just mark data as dirty so a save does something
void updateEE()
{
  eedirty=1;
}

// Main loop
void loop()
{
  Blynk.run();   // Keep Blynk events running
  yield();   // may not be necessary, but trying to stop EEPROM crashes
}


// The Joystick on Blynk is set to virtual pins 0 and 1
// The UI defines the limits as 1000-2000
// Read first axis
BLYNK_WRITE(V0)
{
  int c1;
  c1=param.asInt();
  if (c1!=curr1)  // only send changes
  {
    curr1=c1;
    updates1();                                                           
  }
}

// Read second axis
BLYNK_WRITE(V1)
{
  int c2=param.asInt();
  if (c2!=curr2)   // only send changes
  {
    curr2=c2;
    updates2();
  }
}


// Set new offset
BLYNK_WRITE(V2)
{
  if (!param.asInt()) return;
  off1=1500-curr1;
  off2=1500-curr2;
  updateservos();
  updateEE();
}

// Zero offset
BLYNK_WRITE(V3)
{
  if (!param.asInt()) return;
  off1=off2=0;
  updateservos();
  updateEE();
}

// Swap axis

BLYNK_WRITE(V4)
{
  if (!param.asInt()) return;
  raxis=!raxis;
// I attempted to detach the servos and reattach them but that
// did not seem to work
  updateservos();
  updateEE();
}

// Flip S1
BLYNK_WRITE(V5)
{
  if (!param.asInt()) return;
  s1flip=!s1flip;
  updates1();
  updateEE();
}

// Flip S2
BLYNK_WRITE(V6)
{
  if (!param.asInt()) return;
  s2flip=!s2flip;
  updates2();
  updateEE(); 
}

// Save to EEProm
BLYNK_WRITE(V7)
{
  if (!param.asInt()) return;
  _updateEE();
}

