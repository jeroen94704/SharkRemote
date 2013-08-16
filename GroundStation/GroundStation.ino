#include <JeeLib.h>

// Motor pins
int m11 = 3;
int m12 = 5;
int m21 = 6;
int m22 = 9;
int servoPin = 8;

int PWMval = 80;

// buffer for an outgoing data packet
static byte outBuf[RF12_MAXDATA], outDest;
static char outCount = -1;

char payload[] = "CMDD05XXX"; // Initialize to the right size
byte needToSend;

void setup()
{
  Serial.begin(57600); 

  rf12_initialize(1, RF12_868MHZ, 33);
}

void sendAnalogValue(int portNum, byte value)
{
  sprintf(payload, "WD%02d%03d\0", portNum, value);
  rf12_sendStart(0, payload, sizeof payload);
  rf12_sendWait(0);
  while (!rf12_canSend()) rf12_recvDone();
}

void loop(){

    // RFM12 loop runner, don't report acks
    rf12_recvDone();
    
    if (rf12_canSend()) 
    {
        needToSend = 0;

        Serial.println("Going one way");
        sendAnalogValue(m11, PWMval);
        sendAnalogValue(m12, 0);
        sendAnalogValue(m21, 0);
        sendAnalogValue(m22, PWMval);
        sendAnalogValue(servoPin, 180);
        delay(1000);
         
        Serial.println("Stopping");
        sendAnalogValue(m11, 0);
        sendAnalogValue(m12, 0);
        sendAnalogValue(m21, 0);
        sendAnalogValue(m22, 0);
        sendAnalogValue(servoPin, 120);
        delay(2000);
      
        Serial.println("Going the other way");
        sendAnalogValue(m11, 0);
        sendAnalogValue(m12, PWMval);
        sendAnalogValue(m21, PWMval);
        sendAnalogValue(m22, 0);
        sendAnalogValue(servoPin, 60);
        delay(1000);
      
        Serial.println("Stopping again");
        sendAnalogValue(m11, 0);
        sendAnalogValue(m12, 0);
        sendAnalogValue(m21, 0);
        sendAnalogValue(m22, 0);
        sendAnalogValue(servoPin, 120);
        delay(2000);        
    }    
}
