#include <JeeLib.h>
#include "SoftwareServo.h"
 
// Servo stuff
SoftwareServo servo;
const int servoPin = 8;

// Motor pins
const int m11 = 3;
const int m12 = 5;
const int m21 = 6;
const int m22 = 9;

int PWMval = 64;
int desiredServoPos = 120;
float currentServoPos = desiredServoPos;

int nodeID = 1;

// the setup routine runs once when you press reset:
void setup() 
{    
  rf12_initialize(1, RF12_868MHZ, 33);

  servo.attach(servoPin);
}

boolean parsePayload(volatile uint8_t* data, int len)
{
  if(strncmp((const char *)data, "WD", 2) == 0)   // Handle Write command. Only accept digital pins
  {
    char pinID[3];
    strncpy(pinID, (const char*)data+2, 2);      
    pinID[2] = '\0';

    char value[4];
    strncpy(value, (const char*)data+4, 3);      
    value[3] = '\0';

    int pinNr = atoi(pinID);
    switch(pinNr)
    {
    // Process writes to the motor pins and servo pin
    case m11:
    case m12:
    case m21:
    case m22:
      analogWrite(pinNr, atoi(value));
      break;
    case servoPin:
      desiredServoPos = atoi(value);
      break;
    }
#ifdef RXCOMMANDS
    Serial.print("Write, pin = ");
    Serial.print(pinID);
    Serial.print(", value = ");
    Serial.print(value);
    Serial.println(".");
#endif
  }
  else if(strncmp((const char *)data, "MD", 1) == 0) // Handle Mode command. Only accept digital pins
  {
    // Extract the numeric part of the pinID
    char pinID[3];
    strncpy(pinID, (const char*)data+2, 2);      
    pinID[2] = '\0';
    int pinNr = atoi(pinID);

    char value[4];
    strncpy(value, (const char*)data+4, 3);      
    value[3] = '\0';

    pinMode(pinNr, (atoi(value) == 0) ? INPUT : OUTPUT);

#ifdef RXCOMMANDS
    Serial.print("Mode. pin = "); 
    Serial.print(pinID);
    Serial.print(", value = ");
    Serial.print(value);
    Serial.println(".");
#endif

  }
  else
  {
    Serial.println("couldn't parse incoming message");
  }
}

void processIncoming()
{
  if (rf12_recvDone() && rf12_crc == 0) 
  {
    parsePayload(rf12_data, rf12_len);
  }
}

// the loop routine runs over and over again forever:
void loop()
{
  processIncoming();

  servo.write(currentServoPos);
  
  if(currentServoPos > desiredServoPos)
    currentServoPos-=0.02;
  if(currentServoPos < desiredServoPos)
    currentServoPos+=0.02;

  SoftwareServo::refresh();  
}
