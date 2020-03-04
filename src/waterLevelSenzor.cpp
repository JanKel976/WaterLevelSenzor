#include "header.h"
/*
#include <arduino.h>
#include <Adafruit_NeoPixel.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <printf.h>
#include <SPI.h>
*/

RF24 radio(9,10);                            // Set up nRF24L01 radio on SPI bus plus pins 7 & 8
/*** Topology ***/
RF24Network network(radio); 

const uint16_t this_node = 012;        // Address of our node in Octal format
const uint16_t target_node = 02;       // Address of the other node in Octal format
const unsigned long interval = 2000;
unsigned long last_sent;             // When did we last send?
unsigned long packets_sent; 
//byte addresses[][6] = {"1Node","2Node"};     // Radio pipe addresses for the 2 nodes to communicate.


#define scantime 10000
#define DISPLAYPIN 2 
#define NUMPIXELS 8 // Popular NeoPixel ring size
#define SourcePoint "RaiTan"
Adafruit_NeoPixel pixels(NUMPIXELS, DISPLAYPIN, NEO_GRB);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

void neopixel(uint8_t level);
void sendingToPumpControl(float level);
void radiosetup();
void unusedPins();
void debugInfoSetup();
uint8_t levelReader();
uint8_t ledtesting(uint8_t level);
uint8_t receiver();

struct dataPak{char type[7];float value;};
dataPak toBeSendedPak;
dataPak ReceivedPak;

//struct RecvPak{char source[7];char target[7];char type[7];float value;}acknowledgeData;

void setup() 
{
  unusedPins();
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  SPI.begin();
  //strncpy(toBeSendedPak.this_node,SourcePoint,7);//SourcePoint je nazov tohto pointu urceny v header pomocou #define
  //strncpy(toBeSendedPak.target_node,"P1Cont",7);
  //strncpy(toBeSendedPak.type,"wtrlev",7);
  
  float level = 10.5;  
  toBeSendedPak.value = level;
  radiosetup();
  debugInfoSetup();
}

void loop() 
{
  static uint8_t level;
  static uint8_t prevlevel;
  static uint64_t prevtime;
  if (millis()-prevtime > scantime)
    {
      //level=levelReader();
      level=ledtesting(level);
      prevtime = millis();
    }
  
  if(level!= prevlevel)
    {
      prevlevel = level;
      sendingToPumpControl(level);
      //neopixel(level);
      Serial.print("50 level = ");
      Serial.println(level);
    }
  receiver();
   
}

void sendingToPumpControl(float level)
{ /****************** Ping Out Role ***************************/  
    network.update();                          // Check the network regularly
    toBeSendedPak.value = level;
    strncpy(toBeSendedPak.type,"wtrlev",7);
    unsigned long now = millis();              // If it's time to send a message, send it!
    last_sent = now;
    Serial.print("Sending...");
    //dataPak toBeSendedPak = { millis(), packets_sent++ };
    RF24NetworkHeader header(/*to node*/ target_node);
    header.type=1;
    bool ok = network.write(header,&toBeSendedPak,sizeof(toBeSendedPak));
    if (ok)
      {Serial.println("ok.");Serial.print("sended type ");Serial.print(toBeSendedPak.type);}
    else
      Serial.println("failed.");
}  

void radiosetup()
{
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  // Open a writing and reading pipe on each radio, with opposite addresses
    //radio.openWritingPipe(addresses[0]);
    //radio.openReadingPipe(1,addresses[1]);
  //radio.startListening();
}

void unusedPins()
{
  pinMode (0, INPUT_PULLUP);
  pinMode (1, INPUT_PULLUP);
  pinMode (3, INPUT_PULLUP);
  pinMode (4, INPUT_PULLUP);
  pinMode (5, INPUT_PULLUP);
  pinMode (6, INPUT_PULLUP);  // reserve output
  pinMode (A1, OUTPUT); digitalWrite(A1, HIGH);
  pinMode (A2, OUTPUT); digitalWrite(A2, HIGH);
  pinMode (A3, OUTPUT); digitalWrite(A3, HIGH);
  pinMode (A6, OUTPUT); digitalWrite(A6, HIGH);
  pinMode (A7, OUTPUT); digitalWrite(A7, HIGH);
}

uint8_t levelReader()
{
  bool a = digitalRead(9);
  bool b = digitalRead(7);
  bool c = digitalRead(6);
  Serial.print(a);Serial.print(b);Serial.print(c);Serial.print("  ");
  uint8_t level = a*4+b*2+c;
  return level;
  //return 7; 
}

void neopixel(uint8_t level)
{ 
  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<level; i++) 
    { // For each pixel...
      //if (level == 0)pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      //if (level > 0)pixels.setPixelColor(i, pixels.Color(0, 20, 0));
      pixels.setPixelColor(i, pixels.Color(0, 20, 0));
      pixels.show();
      //delay(DELAYVAL);
    }
}

uint8_t ledtesting(uint8_t level)
  {
  static bool direction = LOW;

  if(direction == LOW && level <= 8)
      {
        if(level < 8)++level;
        else direction = HIGH;
        return level;
      }

  else if (direction == HIGH && level > 0){level=level-1;return level;}

  else {direction = LOW; return 0;}

  //else {direction = LOW;return level;}

  //Serial.print(level);Serial.print("   ");Serial.print(direction);Serial.print("   ");Serial.println(millis());

}

void debugInfoSetup()
{
  printf_begin();
  radio.printDetails();
  Serial.print("Is chip connected" );Serial.println(radio.isChipConnected());
  Serial.print("Failure detected" );Serial.println(radio.failureDetected);
}

uint8_t receiver()
{   
  
  //radio.startListening();
  //delay(20);
  network.update();    
    //byte pipeNo;              // Declare variables for the pipe
  while ( network.available() ) {     // Is there anything ready for us?
    
    RF24NetworkHeader header;        // If so, grab it and print it out
    //payload_t payload;
    network.read(header,&ReceivedPak,sizeof(ReceivedPak));
    Serial.print("Received packet #");
    Serial.print(ReceivedPak.value);
    Serial.print(" at ");
    return ReceivedPak.value; //Serial.println(payload.ms);
  }
   return 99; 
  //else{Serial.print("121 no radio available");return 99;}
}
/*
void acknowledgeLoopControl(unsigned long time,unsigned long started_waiting_at)
{
  Serial.print(acknowledgedData.source);Serial.print("  ");
  Serial.print(acknowledgedData.target);Serial.print("  ");
  Serial.print(acknowledgedData.type);Serial.print("  ");
  Serial.println(acknowledgedData.value);Serial.print("  ");
  Serial.print("roundtrip delay = ");Serial.println(time-started_waiting_at);
}
*/
