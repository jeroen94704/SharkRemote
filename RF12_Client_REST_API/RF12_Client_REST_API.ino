// Arduino demo sketch for testing RFM12B + ethernet
// 2010-05-20 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// Listens for RF12 messages and displays valid messages on a webpage
// Memory usage exceeds 1K, so use Atmega328 or decrease history/buffers
//
// This sketch is derived from RF12eth.pde:
// May 2010, Andras Tucsni, http://opensource.org/licenses/mit-license.php
 
#include <EtherCard.h>

#define DEBUG   1   // set to 1 to display free RAM on web page
#define SERIAL  1   // set to 1 to show incoming requests on serial port

// ethernet interface mac address - must be unique on your network
static byte mymac[] = { 0x00, 0x04, 0xA3, 0x21, 0xCA, 0x38 };

// ethernet interface static fallback ip address, in case DHCP fails
static byte myip[] = { 192,168,0,109 };

// gateway ip address, in case DHCP fails
static byte gwip[] = { 192,168,0,2};

static BufferFiller bfill;  // used as cursor while filling the buffer
byte Ethernet::buffer[1000];   // tcp/ip send and receive buffer

char okHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n";

char badRequest[] PROGMEM = 
    "HTTP/1.0 400 Bad Request\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h1>400 Bad Request</h1>";
    
class PinControl
{
  public:
    PinControl(int num)
    {
      pinNum = num;
    }
    
    void setMode(int m)
    {
      mode = m;
      pinMode(pinNum, mode);
    }
    
    void setValue(int value)
    {
      switch(pinNum)
      {
        case 3:
        case 5:
        case 6:
        case 9:
        case 10:
        case 11:
          analogWrite(pinNum, value); // Implicitly set the mode to OUTPUT
        break;
        
        default:
          if(mode == OUTPUT)
          {
            digitalWrite(pinNum, value == 0 ? LOW : HIGH);
          }
        break;
      }
    }
    
    int getMode()
    {
      return mode;
    }
    
    int getValue()
    {
      if(mode == INPUT || mode == INPUT_PULLUP)
      {
        return digitalRead(pinNum);
      }
    }
    
  private:
    int pinNum;
    int mode;
    int value; 
};

#define NUM_PINS 28

PinControl* pin[NUM_PINS];

void setup()
{
#if SERIAL
    Serial.begin(57600);
    Serial.println("\n[etherNode]");
#endif
    if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
      Serial.println( "Failed to access Ethernet controller");
    //if (!ether.dhcpSetup("Nanode"))
    {
      Serial.println("DHCP failed. Falling back to static");
      ether.staticSetup(myip, gwip);
    }
     
#if SERIAL
    ether.printIp("IP: ", ether.myip);
#endif
  
    for(int i=0; i<NUM_PINS; i++)
    {
      pin[i] = new PinControl(i);
    }
}

int getIntArg(const char* data, const char* key, int value =-1) {
    char temp[10];
    if (ether.findKeyVal(data, temp, sizeof(temp), key) > 0)
        value = atoi(temp);
    return value;
}

// Assuming a url of the form /pins/<pinnr>?... , Starting at the provided offset, extract the pin number
int getPinNumber(const char* data, byte offset) {
    int pin = -1;
    int numChars = 0;

    char* untilPtr = strchr(data + offset, '?');

    if (untilPtr != NULL)
    {
      numChars = untilPtr - (data + offset);
    }
    else
    {
      untilPtr = strchr(data + offset, ' ');
      numChars = untilPtr - (data + offset);
    }

    char temp[2];
    strncpy(temp, data + offset, numChars);
    pin = atoi(temp);
    return pin;
}

void getRequest(const char* data, BufferFiller& buf)
{
  // Check if "pins" is followed by a '/', to ensure proper formatting
  if (data[9] == '/')
  {
    // Get the pin number from the request
    int pinNr = getPinNumber(data, 10);

    if(pinNr < NUM_PINS && (pin[pinNr]->getMode() == INPUT || pin[pinNr]->getMode() == INPUT_PULLUP))
    {
      buf.emit_p(PSTR("$F\r\n { \"pin\": { \"nr\" : \"$D\" \"value\" :  \"$D\" } }"), okHeader, pinNr, pin[pinNr]->getValue());
    }
    else
    {
      bfill.emit_p(PSTR("$F"), badRequest);
    }
  }
}

void putRequest(const char* data, BufferFiller& buf)
{
  // Check if "pins" is followed by a '/', to ensure proper formatting
  if (data[9] == '/')
  {
    #ifdef DEBUG
    Serial.println("Found a '/' where expected");
    #endif

    // Get the pin number from the request
    int pinNr = getPinNumber(data, 10);
    char temp[20];
    
    if(pinNr < NUM_PINS)
    {
      #ifdef DEBUG
      Serial.println("Processing put value request");
      #endif
      
      if (pin[pinNr]->getMode() == OUTPUT && ether.findKeyVal(data+12, temp, sizeof(temp), "value") > 0)
      {
        int value = getIntArg(data+12, "value");
        #ifdef DEBUG
        Serial.print("Found value ");
        Serial.println(value);
        #endif
        
        pin[pinNr]->setValue(value);
        bfill.emit_p(PSTR("$F\r\n <h1>200 OK</h1>"), okHeader);
      }
      else if (ether.findKeyVal(data+12, temp, sizeof(temp), "mode") > 0)
      {
        #ifdef DEBUG
        Serial.print("Setting pin ");
        Serial.print(pinNr);
        Serial.print(" to mode ");
        Serial.println(temp);
        #endif
        
        if(strcmp(temp, "input") == 0)
        {
          pin[pinNr]->setMode(INPUT_PULLUP);
        }
        else if(strcmp(temp, "output") == 0)
        {
          pin[pinNr]->setMode(OUTPUT);
        }       
        
        bfill.emit_p(PSTR("$F\r\n <h1>200 OK</h1>"), okHeader);
      }
      else
      {
        #ifdef DEBUG
        Serial.println("Unrecognized PUT request!");
        #endif
        bfill.emit_p(PSTR("$F"), badRequest);
      }    
    }
  }
}

void loop()
{
    word len = ether.packetReceive();
    word pos = ether.packetLoop(len);
    
    // check if valid tcp data is received
    if (pos) 
    {
        bfill = ether.tcpOffset();
        char* data = (char *) Ethernet::buffer + pos;
#if SERIAL
        Serial.println(data);
#endif
        // receive buf hasn't been clobbered by reply yet
        if (strncmp("GET /pins/", data, 10) == 0)
        {
          if(strchr(data, '?') == NULL)
          {
            getRequest(data, bfill);
          }
          else
          {
            putRequest(data, bfill);
          }
        }
        else
        {
            bfill.emit_p(PSTR("$F"), badRequest);
        }
        ether.httpServerReply(bfill.position()); // send response data
    }
}
