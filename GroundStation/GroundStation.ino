#include <EtherCard.h>
#include <JeeLib.h>

#define DEBUG 1 // set to 1 to display free RAM on web page
#define SERIAL 1 // set to 1 to show incoming requests on serial port

// Motor pins
#define m11 3
#define m12 5
#define m21 6
#define m22 9
#define servoPin 8

#define PWMval  80

#define propMinVal 15
#define propMaxVal 100

#define servoMinVal 30
#define servoMaxVal 180

// Pin ID's
#define unknown 0
#define lprop 1
#define rprop 2
#define servo 3

// buffer for an outgoing data packet
static byte outBuf[RF12_MAXDATA], outDest;
static char outCount = -1;

char payload[10];

// supertweet.net username:password in base64
#define KEY   "U2hhcmtUd2VldDE6a2Fhc2thYXM="
#define API_URL "/1.1/statuses/update.json"

// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

char website[] PROGMEM = "api.supertweet.net";

static BufferFiller bfill; // used as cursor while filling the buffer
byte Ethernet::buffer[500];
Stash stash;
uint8_t runTime;

char okHeader[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n";

char badRequest[] PROGMEM =
    "HTTP/1.0 400 Bad Request\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<h1>400 Bad Request</h1>";

static void sendToTwitter () {
  // generate two fake values as payload - by using a separate stash,
  // we can determine the size of the generated message ahead of time
  byte sd = stash.create();
  
  // Construct the Twitter status message containing the IP address
  stash.print("status=My current IP is: ");
  for (byte i = 0; i < 4; ++i) 
  {
    stash.print( ether.myip[i], DEC );
    if (i < 3)
    stash.print('.');
  }
  stash.println("");
  stash.save();

  // generate the header with payload - note that the stash size is used,
  // and that a "stash descriptor" is passed in as argument using "$H"
  Stash::prepare(PSTR("POST /1.1/statuses/update.json HTTP/1.1" "\r\n"
                     "Host: $F" "\r\n"
                     "Authorization: Basic $F" "\r\n"
                     "User-Agent: Arduino EtherCard lib" "\r\n"                        
                     "Content-Length: $D" "\r\n"
                     "Content-Type: application/x-www-form-urlencoded" "\r\n"
                     "\r\n"
                     "$H"),
                     website, PSTR(KEY), stash.size(), sd); 

  // send the packet - this also releases all stash buffers once done
  Serial.println( "Sending IP address to twitter");
  ether.tcpSend();
}

void setup()
{
  Serial.begin(57600); 
  Serial.print("Initializing RF12 ... ");
  rf12_initialize(1, RF12_868MHZ, 33);
  Serial.println("done");

  Serial.print("\nInitializing ethernet ... ");

  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  Serial.println("done");

  ether.printIp("IP:  ", ether.myip);


  if (!ether.dnsLookup(website))
    Serial.println("DNS lookup failed");

  // Send the current IP to twitter (@SharkTweet1)
  sendToTwitter();
}

void sendAnalogValue(int portNum, byte value)
{
  sprintf(payload, "WD%02d%03d\0", portNum, value);
  Serial.print("Sending command ");
  Serial.print(payload);
  Serial.println(" over wireless");
  rf12_sendStart(0, payload, sizeof payload);
  rf12_sendWait(0);
  while (!rf12_canSend()) rf12_recvDone();
}

// Assuming a url of the form /pins/<pinid>?... , Starting at the provided offset, extract the pin id
int getPinId(const char* data, byte offset) {
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

    char temp[6];
    if(numChars < 6)
    {
      strncpy(temp, data + offset, numChars);
      temp[numChars] = '\0';
    #ifdef DEBUG
    Serial.print("pindid = ");
    Serial.println(temp);
    #endif
      if(strcmp(temp, "lprop") == 0)
        return lprop;
      else if(strcmp(temp, "rprop") == 0)
        return rprop;
      else if(strcmp(temp, "servo") == 0)
        return servo;
    }

    return unknown;
}

int getIntArg(const char* data, const char* key, int value =-1) {
    char temp[10];
    if (ether.findKeyVal(data, temp, sizeof(temp), key) > 0)
        value = atoi(temp);
    return value;
}

void processPropVal(int value, int pin1, int pin2)
{
  int thePin;
  
  if(value == 0) 
  {            
    sendAnalogValue(pin1, 0);
    sendAnalogValue(pin2, 0);
    return;
  }
  
  if(value > 0)
  {
    thePin = pin1;
  } 
  else
  {
    value *= -1;
    thePin = pin2;
  }
  
  // Clip to 0-100
  value = min(value, 100);
  value = max(value, 0);
  
  // Remap value from 1-100 to 15-100 (which is the useful range for the props)
  value = map(value, 1, 100, propMinVal, propMaxVal);
  sendAnalogValue(thePin, value); 
}

void putRequest(const char* data, BufferFiller& buf)
{
  // Check if "pins" is followed by a '/', to ensure proper formatting
  if (data[11] == '/')
  {
    #ifdef DEBUG
    Serial.println("Found a '/' where expected");
    #endif

    // Get the pin number from the request
    int pinId = getPinId(data, 12);
    int thePin;
    char temp[20];
    
    if(pinId != unknown)
    {
      #ifdef DEBUG
      Serial.println("Processing put value request");
      #endif
      
      if (ether.findKeyVal(data+18, temp, sizeof(temp), "value") > 0)
      {
        int value = getIntArg(data+18, "value");
        #ifdef DEBUG
        Serial.print("Found value ");
        Serial.println(value);
        #endif
        
        switch(pinId) {
          case lprop :
            processPropVal(value, m11, m12);
          break;
          case rprop :
            processPropVal(value, m21, m22);
          break;
          case servo : 
            // Clip to 0-100
            value = min(value, 100);
            value = max(value, 0);
          
            sendAnalogValue(servoPin, map(value, 0, 100, servoMinVal, servoMaxVal));
          break;
          default:
            #ifdef DEBUG
            Serial.println("Unrecognized PUT request!");
            #endif
            bfill.emit_p(PSTR("$F"), badRequest);
            return;
          break;
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

// To test, use :  curl -i -H "Accept: application/json" -X GET http://192.168.0.110/1/pins/servo?value=180

void loop()
{    
    // RFM12 loop runner, don't report acks
    rf12_recvDone();

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
        if (strncmp("GET /1/pins/", data, 12) == 0)
        {
          putRequest(data, bfill);
        }
        else
        {
          bfill.emit_p(PSTR("$F"), badRequest);
        }
        ether.httpServerReply(bfill.position()); // send response data
    }   
}
