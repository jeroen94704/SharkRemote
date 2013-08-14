#include <JeeLib.h>
 
// Motor pins
int m11 = 3;
int m12 = 5;
int m21 = 6;
int m22 = 9;

int PWMval = 64;

int nodeID = 1;

// the setup routine runs once when you press reset:
void setup() 
{    
  rf12_initialize(nodeID, RF12_868MHZ, 33);
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
    case 0:
    case 1:
    case 2:
    case 4:
    case 7:
    case 8:
    case 12:
    case 13:
      digitalWrite(pinNr, (atoi(value) == 0) ? LOW : HIGH);
      break;  

    case 3:
    case 5:
    case 6:
    case 9:
    case 10:
    case 11:
      analogWrite(pinNr, atoi(value));
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

  analogWrite(m11, 0);
  analogWrite(m12, PWMval);
  analogWrite(m21, 0);
  analogWrite(m22, PWMval);
  delay(1000);


  analogWrite(m11, 0);
  analogWrite(m12, 0);
  analogWrite(m21, 0);
  analogWrite(m22, 0);
  delay(2000);

  analogWrite(m11, PWMval);
  analogWrite(m12, 0);
  analogWrite(m21, PWMval);
  analogWrite(m22, 0);
  delay(1000);

  analogWrite(m11, 0);
  analogWrite(m12, 0);
  analogWrite(m21, 0);
  analogWrite(m22, 0);
  delay(2000);
  
}
