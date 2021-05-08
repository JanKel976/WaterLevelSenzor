
#include <arduino.h>
#include <Adafruit_NeoPixel.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <RF24Network.h>
#include <printf.h>
#include <SPI.h>


RF24 radio(9,10);       0,3
                     // Set up nRF24L01 radio on SPI bus plus pins 7 & 8
/*** Topology ***/
RF24Network network(radio); 

const uint16_t this_node = 012;        // Address of our node in Octal format
const uint16_t to_node = 02;       // Address of the other node in Octal format
const unsigned long interval = 2000;

#define scantime 1000
#define DISPLAYPIN 2 
#define NUMPIXELS 8 // Popular NeoPixel ring size
#define SourcePoint "RaiTan"
Adafruit_NeoPixel pixels(NUMPIXELS, DISPLAYPIN, NEO_GRB);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

void neopixel(uint8_t level);
void sendingToPumpControl(float level);
void radiosetup();
void pinSettings();
void debugInfoSetup();
uint8_t levelReader();\
uint8_t ledtesting(uint8_t level);
void receiver();

struct dataPak{float value1;float value2;};
dataPak toBeSendedPak;
dataPak ReceivedPak;
bool debugger;

void setup() 
{
  debugger = 0;
  pinSettings();
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  SPI.begin();
  float level = 0;  
  toBeSendedPak.value1 = level;
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
      if(debugger == 0)level=levelReader();
      else level = ledtesting(level);
      Serial.print("line 69 level = ");Serial.println(level);
      prevtime = millis();
    }
  
  if(level!= prevlevel)
    {
      prevlevel = level;
      sendingToPumpControl(level);
   

  Serial.println("levelchange ");
      
      
    }
    neopixel(level);
  //receiver();
}

void sendingToPumpControl(float level)
{ /****************** Ping Out Role ***************************/  
    network.update();                          // Check the network regularly
    toBeSendedPak.value1 = level;
    if(debugger == 1){Serial.print("Sending...");}
    RF24NetworkHeader header(/*to node*/ to_node);
    //Serial.print("95 target node  ");Serial.println(header.to_node);
    header.type=1;
    bool ok = network.write(header,&toBeSendedPak,sizeof(toBeSendedPak));
    if (ok && debugger == 1)
      {Serial.println("ok.");}
    else
      if(debugger == 1){Serial.println("failed.");}
}  

void radiosetup()
{
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
}

void pinSettings()
{
  pinMode (0, INPUT_PULLUP);
  pinMode (1, INPUT_PULLUP);
  pinMode (3, INPUT_PULLUP);
  pinMode (4, INPUT_PULLUP);
  pinMode (5, INPUT_PULLUP);
  pinMode (6, INPUT_PULLUP);  // reserve output
  pinMode(9,INPUT);
  pinMode(10,INPUT);
  pinMode(11,INPUT);
}

uint8_t levelReader()
{
  uint8_t level = 0;
  //Serial.print("line 125 level = ");Serial.println(level);
  if (analogRead(A0)> 950){level = 0;}
  if (analogRead(A0)<=950){level = 1;}
  if (analogRead(A1)<=950){level = 2;}
  if (analogRead(A2)<=950){level = 3;}
  if (analogRead(A3)<=950){level = 4;}
  if (analogRead(A4)<=950){level = 5;}
  if (analogRead(A5)<=950){level = 6;}
  if (analogRead(A6)<=950){level = 7;}
  if (analogRead(A7)<=950){level = 8;}

  Serial.print("analogRead(A0)= ");Serial.println(analogRead(A0));
  Serial.print("analogRead(A1)= ");Serial.println(analogRead(A1));
  Serial.print("analogRead(A2)= ");Serial.println(analogRead(A2));
  Serial.print("analogRead(A3)= ");Serial.println(analogRead(A3));
  Serial.print("analogRead(A4)= ");Serial.println(analogRead(A4));
  Serial.print("analogRead(A5)= ");Serial.println(analogRead(A5));
  Serial.print("analogRead(A6)= ");Serial.println(analogRead(A6));
  Serial.print("analogRead(A7)= ");Serial.println(analogRead(A7));

  //Serial.print("  line 134 analogread2 = ");Serial.print(analogRead(A2));Serial.print("  ");
  //Serial.print("  line 134 level = ");Serial.println(level);
  //neopixel(level);

  //bool b = digitalRead(10);
  //bool c = digitalRead(11);
  //Serial.print("A7 = ");Serial.print(analogRead(A7));Serial.print("  ");
  //Serial.print("line 138 level = ");Serial.println(level);
  //Serial.print(b);Serial.print(c);Serial.print("  ");
  //uint8_t level = a*4+b*2+c;
  
  return level;
}

void neopixel(uint8_t level)
{ 
  pixels.clear(); // Set all pixel colors to 'off'
  
  

  if (level == 0)
    { static uint32_t prevtimelong;
      //Serial.print("line 142 neopixel loop level = 0 ");
      //Serial.print(millis()-prevtimelong);
      
      if (millis() - prevtimelong < 1500)
        {
          pixels.setPixelColor(0, pixels.Color(0, 20, 0));
        }
      else if(millis() - prevtimelong >= 1500 && millis() - prevtimelong < 6000 )
        {
          //Serial.print("  pixelclear  ");
          pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        }
      else if (millis() - prevtimelong >= 6000)prevtimelong = millis();

    }
  else{
      for(int i=0; i<level; i++) 
        { // For each pixel...
          //if (level == 0)pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          //if (level > 0)pixels.setPixelColor(i, pixels.Color(0, 20, 0));
          pixels.setPixelColor(i, pixels.Color(0, 20, 0));
          //pixels.show();
          //delay(DELAYVAL);
        }
      }
pixels.show();
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

  Serial.print(level);Serial.print("   ");Serial.print(direction);Serial.print("   ");Serial.println(millis());

}

void debugInfoSetup()
{
  printf_begin();
  radio.printDetails();
  Serial.print("Is chip connected" );Serial.println(radio.isChipConnected());
  Serial.print("Failure detected" );Serial.println(radio.failureDetected);
}

void receiver()
{   
  network.update();    
  while ( network.available() )
    {     // Is there anything ready for us?
      RF24NetworkHeader header;        // If so, grab it and print it out
      network.read(header,&ReceivedPak,sizeof(ReceivedPak));
      Serial.print("Received packet #");
      Serial.print(ReceivedPak.value1);
      Serial.print(" at ");
    }
}

