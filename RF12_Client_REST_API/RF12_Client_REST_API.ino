// Arduino demo sketch for testing RFM12B + ethernet
// 2010-05-20 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// Listens for RF12 messages and displays valid messages on a webpage
// Memory usage exceeds 1K, so use Atmega328 or decrease history/buffers
//
// This sketch is derived from RF12eth.pde:
// May 2010, Andras Tucsni, http://opensource.org/licenses/mit-license.php
 
#include <EtherCard.h>
#include <JeeLib.h>
#include <avr/eeprom.h>

#define DEBUG   1   // set to 1 to display free RAM on web page
#define SERIAL  1   // set to 1 to show incoming requests on serial port

#define CONFIG_EEPROM_ADDR ((byte*) 0x10)

// ethernet interface mac address - must be unique on your network
static byte mymac[] = { 0x00, 0x04, 0xA3, 0x21, 0xCA, 0x38 };

// ethernet interface static fallback ip address, in case DHCP fails
static byte myip[] = { 192,168,0,109 };

// gateway ip address, in case DHCP fails
static byte gwip[] = { 192,168,0,2};

// buffer for an outgoing data packet
static byte outBuf[RF12_MAXDATA], outDest;
static char outCount = -1;

#define NUM_MESSAGES  10    // Number of messages saved in history
#define MESSAGE_TRUNC 15    // Truncate message payload to reduce memory use

static BufferFiller bfill;  // used as cursor while filling the buffer

static byte next_msg;       // pointer to next rf12rcvd line
static word msgs_rcvd;      // total number of lines received modulo 10,000

byte Ethernet::buffer[1000];   // tcp/ip send and receive buffer

MilliTimer sendTimer;
char payload[] = "CMDD04XXX";
byte needToSend;

int rxLed = 5;
int txLed = 6;
bool invertLed = true;
int intensity = 0;

void sendLed(bool on)
{
  digitalWrite(txLed, (invertLed ? on : !on) ? LOW : HIGH);  
}

void recvLed(bool on)
{
  digitalWrite(rxLed, (invertLed ? on : !on) ? LOW : HIGH);  
}

void processIncoming()
{
  if (rf12_recvDone() && rf12_crc == 0) 
  {
    // @todo ignore for now
  }
}

void setup()
{
#if SERIAL
    Serial.begin(57600);
    Serial.println("\n[etherNode]");
#endif
    rf12_initialize(31, RF12_868MHZ, 33);
    
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

    pinMode(rxLed, OUTPUT);
    pinMode(txLed, OUTPUT);
    
    sendLed(false);
    recvLed(false);    
}

char okHeader[] PROGMEM = 
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n";

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
        if (strncmp("GET /", data, 5) == 0)
            getValue(bfill);
        else if (strncmp("POST /", data, 6) == 0)
            setValue(data, bfill);
        ether.httpServerReply(bfill.position()); // send web page data
    }

    // RFM12 loop runner, don't report acks
    if (rf12_recvDone() && rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0) {
        history_rcvd[next_msg][0] = rf12_hdr;
        for (byte i = 0; i < rf12_len; ++i)
            if (i < MESSAGE_TRUNC) 
                history_rcvd[next_msg][i+1] = rf12_data[i];
        history_len[next_msg] = rf12_len < MESSAGE_TRUNC ? rf12_len+1
                                                         : MESSAGE_TRUNC+1;
        next_msg = (next_msg + 1) % NUM_MESSAGES;
        msgs_rcvd = (msgs_rcvd + 1) % 10000;

    }
    
    if (sendTimer.poll(250))
    {
        while (!rf12_canSend()) processIncoming();
      
        sendLed(true);
#ifdef DEBUG                
        Serial.println("Send started");
#endif
        sprintf(payload, "MD06255\0");
        rf12_sendStart(0, payload, sizeof payload);
        rf12_sendWait(0);

        while (!rf12_canSend()) rf12_recvDone();

        sprintf(payload, "BA01255\0");
        rf12_sendStart(0, payload, sizeof payload);
        rf12_sendWait(0);

        while (!rf12_canSend()) rf12_recvDone();

        sprintf(payload, "WD06%03d\0", intensity);
        rf12_sendStart(0, payload, sizeof payload);
        intensity = (intensity+64) % 255;

        sendLed(false);
   }    
}
