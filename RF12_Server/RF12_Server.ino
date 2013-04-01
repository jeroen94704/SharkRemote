// (c) 2012 Jeroen Bouwens
// 
// Server-side sketch, to be run on a Winode, allowing control of said  Winode over the RF12 
// wireless connection
//
// Supported functions:
//    - Set state of digital output pins (including PWM, i.e. analogWrite)
//    - Set input/output mode of digital pins
//    - Values of Analog and Digital input pins are periodically broadcast
//
// WARNING: No security whatsoever is implemented in the wireless protocol, 
// meaning anyone can sniff out the protocol and/or take over control 
//
// ## Protocol description:
// 
// Accepted messages are of the form:
//
// +-+---+---+
// |C|###|XXX|
// +-+---+---+
//
// Where:
//   * C   : Command. Accepted values are "W" (Write value), "M" (pin i/o Mode) or "B" (set Broadcast value mode)
//   * ### : Pin ID. Can be D00..D13 for Digital 0-13 or A00..A05 for Analog 0-5.
//   * XXX : Value. 
//
// The W command sets the value of a pin that is set to output mode. Digital pins 3, 5, 6, 9, 10, and 11 are PWM capable. For these pins, 
// the value be anything from 000-255. For the remaining digital pins, 000 means LOW and non-0 means HIGH.
//
// The M command sets a digital pin to either input or output mode. A value of 0 means Input, any non-0 value means output. Writes to input 
// pins are ignored. Analog pins are always input.
//
// The B command determines if the value of a pin (assuming it is set to input) is periodically broadcast. A value of 0 means not broadcast, 
// any non-0 value means it will be broadcast. 
//
// The module periodically broadcasts the values for input pins, both analog and digital, which have Broadcast Value Mode enabled. The
// messages it sends are of the format:
//
// +---+---+
// |###|XXX|
// +---+---+
//
// Where:
//   * ### : Pin ID. Can be D00..D13 or A00..A05 for Digital 0-13 or Analog 0-5
//   * XXX : Value. For digital input pins, this is either 000 or 255. For analog input pins, this can be any value from 000 or 255.
// 
// If a pin is set to output, and has its BVM set to true (broadcast), the value will NOT be broadcast.


#include <JeeLib.h>
#include <pins_arduino.h>

//#define DEBUG 1 // Print debug messages over the serial line
#define TRACE 1
#define RXCOMMANDS 1 // Print received commands

#ifdef TRACE
  #define TRACEIT(num) Serial.print("trace "); Serial.println(num);
#else
  #define TRACEIT(num)
#endif

int nodeID = 1;

// A flag for each digital input, determines if the value is broadcast
boolean broadcastDigitalInputs[] = 
{
  false, false, false, false,
  false, false, false , false,
  false, false, false, false,
  false, false
};

// Same for analog inputs
boolean broadcastAnalogInputs[] = 
{ 
  false, false, false, false, false, false 
};

// Debug message buffer
char txBuff[6];

// Timer for periodic broadcasting of values
MilliTimer sendTimer;
int broadcastPeriod = 500;

void setup ()
{
  Serial.begin(57600);
  Serial.println("Winode RF12 Server");
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
  else if(strncmp((const char *)data, "B", 1) == 0) // Handle Broadcast command. 
  {
    char pinID[3];
    strncpy(pinID, (const char*)data+2, 2);      
    pinID[2] = '\0';

    char value[4];
    strncpy(value, (const char*)data+4, 3);      
    value[3] = '\0';    

    int pinNr = atoi(pinID);

    switch(data[1])
    {
    case 'A':
      broadcastAnalogInputs[pinNr] = (atoi(value) == 0) ? false : true;
      break;
    case 'B':
      broadcastDigitalInputs[pinNr] = (atoi(value) == 0) ? false : true;
      break;
    }

#ifdef RXCOMMANDS
    Serial.print("Broadcast, pin = ");
    Serial.write(data[1]);
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

int readPinMode(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  if (port == NOT_A_PIN) return OUTPUT; // bad pin!

  volatile uint8_t *reg = portModeRegister(port);

  return (*reg & bit) ? OUTPUT : INPUT;
}

void processIncoming()
{
  if (rf12_recvDone() && rf12_crc == 0) 
  {
    parsePayload(rf12_data, rf12_len);
  }
}

void broadcastInputValues()
{
  for(int i=0; i < 14; i++)
  {
    if(broadcastDigitalInputs[i] && readPinMode(i) == INPUT && rf12_canSend())
    {
      TRACEIT(1);
//      while (!rf12_canSend()) { processIncoming(); delay(10); } // No plain call to recvDone, as this ignores packages
      TRACEIT(2);
      // Broadcast message containing value of analog pin X
      boolean value = digitalRead(i);
      sprintf(txBuff, "D%02d%03d\0", i, value);
#ifdef DEBUG
      Serial.print("Sending message: ");
      Serial.println(txBuff);
#endif
      TRACEIT(5);
      rf12_sendStart(0, txBuff, sizeof txBuff);
      TRACEIT(6);
      rf12_sendWait(0);
      TRACEIT(7);
    }
  }
#ifdef DEBUG
  Serial.println("");
#endif

  for(int i=0; i < 6; i++)
  {
    if(broadcastAnalogInputs[i] && rf12_canSend())
    {
      TRACEIT(3);
//      while (!rf12_canSend()) { processIncoming(); delay(10); } // No plain call to recvDone, as this ignores packages
      TRACEIT(4);
      // Broadcast message containing value of analog pin X
      int value = analogRead(i);
      sprintf(txBuff, "A%02d%03d\0", i, value);
#ifdef DEBUG
      Serial.print("Sending message: ");
      Serial.println(txBuff);
#endif
      TRACEIT(8);
      rf12_sendStart(0, txBuff, sizeof txBuff);
      TRACEIT(9);
      rf12_sendWait(0);
      TRACEIT(10);
    }
  }
}

void loop () 
{
  processIncoming();

  if (sendTimer.poll(broadcastPeriod))
  {
    broadcastInputValues(); 
  }
}



