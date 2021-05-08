
#include <arduino.h>
#include <Adafruit_NeoPixel.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <RF24Network.h>
#include <printf.h>
#include <SPI.h>


RF24 radio(9,10);      // 0,3
                     // Set up nRF24L01 radio on SPI bus plus pins 7 & 8
/*** Topology ***/
RF24Network network(radio); 

const uint16_t this_node = 012;        // Address of our node in Octal format
const uint16_t to_node = 02;       // Address of the other node in Octal format
const unsigned long interval = 2000;

#define scantime 15000 //ako casto sa ma zistovat hladina
#define DISPLAYPIN 2 
#define NUMPIXELS 8 // Popular NeoPixel ring size
#define SourcePoint "RaiTan"  //Rainwater Tank

Adafruit_NeoPixel pixels(NUMPIXELS, DISPLAYPIN, NEO_GRB);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

void neopixel(uint8_t level);
void sendingToPumpControl(byte level);
void radiosetup();
void pinSettings();
void debugInfoSetup();
uint8_t levelReader();\
uint8_t ledtesting(uint8_t level);
void receiver();
int inputReader(byte i);


struct dataPak{byte value1;float value2;};
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
  toBeSendedPak.value1 = 0;
  pixels.setPixelColor(7, pixels.Color(20, 0, 0)); //indikator zapnutia controlera
  pixels.show();
  radiosetup();
  debugInfoSetup();
  levelReader();
  
}

void loop() 
{
  static uint8_t level;
  static uint8_t prevlevel;
  static uint64_t prevtime;

  if (millis()-prevtime > scantime)
    {
      level=levelReader();
      if(debugger ==1){Serial.print("line 65 level = ");Serial.println(level);}
      prevtime = millis();
    }
  Serial.println("line 72 level = ");Serial.println(level);
  if(level!= prevlevel)
    {
      neopixel(level);
      //sendingToPumpControl(level);
      Serial.println("line 73 levelchange ");
      prevlevel = level;
    }
 
  delay(500);
  //receiver();
}

void sendingToPumpControl(byte level)
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
  pinMode (0, INPUT_PULLUP);//D0
  pinMode (1, INPUT_PULLUP);//D1
  pinMode (DISPLAYPIN,OUTPUT); //D2
  pinMode (3,INPUT); //D3
  pinMode (4, INPUT);
  pinMode (5, INPUT);
  pinMode (6, INPUT);  // reserve output
  pinMode(A0,INPUT); //A0
  pinMode(A1,INPUT); //A1
  pinMode(A2,INPUT);  //...
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  pinMode(A5,INPUT);
  pinMode(A6,INPUT);
  pinMode(A7,INPUT); //A7
  //pinMode(10,INPUT);
  //pinMode(11,INPUT);
}

uint8_t levelReader()
{int volts;
  static uint8_t level = 0;

  pixels.setPixelColor(6, pixels.Color(0, 0, 20));
  pixels.show();
  //digitalWrite(relePin,HIGH);//zapni rele privodu napatia do sondy
  for(int i=14;i<=19;i++){
     volts=inputReader(i);
     if(volts<=850){level=i-14;break;}
     Serial.print(i);Serial.print("  line 139 volts = ");Serial.println(volts);
  }
 //digitalWrite(relePin,LOW); //vypni rele privodu napatia do sondy
 // if(level<=7){level=level+1;delay(500);}
 // else level =0;
  pixels.setPixelColor(6, pixels.Color(0, 0, 0));
  pixels.show();
  return level;



  //Serial.print("line 125 level = ");Serial.println(level);
  /*      
  
  uint16_t pin0,pin1,pin2,pin3,pin4,pin5,pin6,pin7;
  pin0=analogRead(A0);delay(100);
  pin1=analogRead(A1);delay(100);
  pin2=analogRead(A2);delay(100);
  pin3=analogRead(A3);delay(100);
  pin4=analogRead(A4);delay(100);
  pin5=analogRead(A5);delay(100);
  pin6=analogRead(A6);delay(100);
  pin7=analogRead(A7);delay(100);

  if (pin0 > 950){level = 0;delay(2);Serial.print(" pin0= ");Serial.println(pin0);}
  if (pin0<=950){level = 1;delay(2);Serial.print(" pin0= ");Serial.println(pin0);}
  if (pin1<=950){level = 2;delay(2);Serial.print(" pin1= ");Serial.println(pin1);}
  if (pin2<=950){level = 3;delay(2);Serial.print(" pin2= ");Serial.println(pin2);}
  if (pin3<=950){level = 4;delay(2);Serial.print(" pin3= ");Serial.println(pin3);}
  if (pin4<=950){level = 5;delay(2);Serial.print(" pin4= ");Serial.println(pin4);}
  if (pin5<=950){level = 6;delay(2);Serial.print(" pin5= ");Serial.println(pin5);}
  if (pin6<=950){level = 7;delay(2);Serial.print(" pin6= ");Serial.println(pin6);}
  if (pin7<=950){level = 8;delay(2);Serial.print(" pin7= ");Serial.println(pin7);}
  
  

  
  Serial.print("analogRead(A1)= ");Serial.println(analogRead(A1));delay(500);
  Serial.print("analogRead(A2)= ");Serial.println(analogRead(A2));delay(500);
  Serial.print("analogRead(A3)= ");Serial.println(analogRead(A3));delay(500);
  Serial.print("analogRead(A4)= ");Serial.println(analogRead(A4));delay(500);
  Serial.print("analogRead(A5)= ");Serial.println(analogRead(A5));delay(500);
  Serial.print("analogRead(A6)= ");Serial.println(analogRead(A6));delay(500);
  Serial.print("analogRead(A7)= ");Serial.println(analogRead(A7));delay(500);

  //Serial.print("  line 134 analogread2 = ");Serial.print(analogRead(A2));Serial.print("  ");
  //Serial.print("  line 134 level = ");Serial.println(level);
  //neopixel(level);

  //bool b = digitalRead(10);
  //bool c = digitalRead(11);
  //Serial.print("A7 = ");Serial.print(analogRead(A7));Serial.print("  ");
  //Serial.print("line 138 level = ");Serial.println(level);
  //Serial.print(b);Serial.print(c);Serial.print("  ");
  //uint8_t level = a*4+b*2+c;
 */ 

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
      for(int i=0; i<=level; i++) 
        { // For each pixel...
          //if (level == 0)pixels.setPixelColor(i, pixels.Color(0, 0, 0));
          //if (level > 0)pixels.setPixelColor(i, pixels.Color(0, 20, 0));
          pixels.setPixelColor(i+2, pixels.Color(0, 20, 0));
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

int inputReader(byte i){
int pinValue;
pinMode(i,INPUT_PULLUP);
delay(1);
pinValue=analogRead(i);
pinMode(i,INPUT);
return pinValue;
}

