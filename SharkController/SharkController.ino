#include <JeeLib.h>
#include "SoftwareServo.h"
 
//#define RXCOMMANDS

// Servo stuff
SoftwareServo servo;
const int servoPin = 8;

// Motor pins
const int m11 = 3;
const int m12 = 5;
const int m21 = 6;
const int m22 = 9;

int PWMval = 64;
int desiredServoPos = 90;
float currentServoPos = desiredServoPos;

int vccCheckCount = 0;
int nodeID = 1;

long readVcc() 
{
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

// the setup routine runs once when you press reset:
void setup() 
{    
#ifdef RXCOMMANDS
  Serial.begin(57600); 
  Serial.print("Initializing RF12 ... ");
#endif
  rf12_initialize(1, RF12_433MHZ, 33);
#ifdef RXCOMMANDS
  Serial.println("done");  
#endif

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
    Serial.print("Supply voltage: ");
    Serial.print(readVcc());
    Serial.println("[mV]");
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
#ifdef RXCOMMANDS    
    Serial.println("couldn't parse incoming message");
#endif    
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
  // Check if the voltage is still sufficient
  bool doProcessing = true;
  if(vccCheckCount++ > 10000)
  {
    doProcessing = readVcc() > 3000;
    vccCheckCount = 0;
  }

  if(doProcessing)
  {
    processIncoming();
  
    servo.write(currentServoPos);
    
    if(currentServoPos > desiredServoPos)
    {
      currentServoPos-=0.04;
    }
    
    if(currentServoPos < desiredServoPos)
    {
      currentServoPos+=0.04;
    }
  
    SoftwareServo::refresh(); 
  }
  else
  {
    // Switch everything off when voltage drops too low
    analogWrite(m11, 0);
    analogWrite(m12, 0);
    analogWrite(m21, 0);
    analogWrite(m22, 0);
    
    servo.detach();
  }
}
