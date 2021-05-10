
#include <arduino.h>
#include <Adafruit_NeoPixel.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <RF24Network.h>
#include <printf.h>
#include <SPI.h>

/******* Set up nRF24L01 radio on SPI bus plus pins 7 & 8 *************/
RF24 radio(9,10);      // 0,3

/*********** Topology of network ************************/
RF24Network network(radio); 

const uint16_t this_node = 012;        // Address of our node in Octal format
const uint16_t to_node = 02;       // Address of the other node in Octal format
const unsigned long interval = 2000;

#define scantime 60000 //ako casto sa ma zistovat hladina
#define DISPLAYPIN 2 
#define RELEPIN 3
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
void signals(bool scanflag,byte _level);

struct dataPak{byte value1;float value2;};
dataPak toBeSendedPak;
dataPak ReceivedPak;
bool debugger;

void setup(){
  pinSettings();
  debugger = 0;
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  SPI.begin();
  toBeSendedPak.value1 = 0; //value1 = level, value2 = rezervovane 
  radiosetup();
  debugInfoSetup();
  levelReader();
}

void loop() 
{
  static uint8_t level;
  static uint8_t prevlevel;
  static uint64_t prevtime;
  static bool scanflag;
 
  if (millis()-prevtime > scantime)  //hladina vody sa scanuje v case scantime
    {
      level=levelReader();
      scanflag=HIGH;           //oznamenie scanovania
      prevtime = millis();     //vynulovanie casu
      sendingToPumpControl(level); //odoslanie do kontrolera pumpy
    }

//if(debugger ==0){Serial.print("line 73 level = ");Serial.println(level);} //pre debugging
 
if(level!= prevlevel)  //pri zmene levelu sa zobrazi zmena
    {
      neopixel(level);
      //if(debugger ==1)Serial.println("line 78 levelchange "); //pre debugging
      prevlevel = level; //nastavenie noveho levelu
    }
signals(scanflag,level);  //signaly funkcii (pre scanovanie modra farba,pre kontrolu loopu blikanie)

if(millis()-prevtime > 2000)scanflag = LOW; // vypnutie oznamenia scanovania
  delay(500);
  receiver();  // prepnutie do modu prijimania
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
  pinMode (RELEPIN,OUTPUT); //D3
  pinMode (0, INPUT_PULLUP);//D0
  pinMode (1, INPUT_PULLUP);//D1
  pinMode (DISPLAYPIN,OUTPUT); //D2
  pinMode (4, INPUT_PULLUP);
  pinMode (5, INPUT_PULLUP);
  pinMode (6, INPUT_PULLUP);  // reserve output
  pinMode(A0,INPUT); //A0
  pinMode(A1,INPUT); //A1
  pinMode(A2,INPUT);  //...
  pinMode(A3,INPUT);
  pinMode(A4,INPUT);
  pinMode(A5,INPUT);
  pinMode(A6,INPUT);
  pinMode(A7,INPUT); //A7
  //pinMode(10,INPUT); // reserved for SPI
  //pinMode(11,INPUT);

}

uint8_t levelReader()
{int volts;
  static uint8_t _level = 0;
  
  for(int i=21;i>=14;i--){
     volts=inputReader(i);
     if(volts<=850){_level=i-13;
     if(debugger ==1){Serial.print(i);Serial.print("  line 139 volts = ");Serial.print(volts);}
     break;
     }
     else _level=0;
  }

  return _level;
}

void neopixel(uint8_t level)
{ 
  pixels.clear(); // Set all pixel colors to 'off'
  
      for(int i=0; i<=level; i++) 
        { 
          pixels.setPixelColor(i-1, pixels.Color(0, 20, 0));
        }
     
pixels.show();
}

void receiver()
{   
  network.update();    
  while ( network.available() )
    {     // Is there anything ready for us?
      RF24NetworkHeader header;        // If so, grab it and print it out
      network.read(header,&ReceivedPak,sizeof(ReceivedPak));
     if(debugger ==1){
      Serial.print("Received packet #");
      Serial.print(ReceivedPak.value1);
      Serial.print(" at ");}
    }
}

int inputReader(byte pin){
    int pinValue;
    Serial.print(" 242 pin ");Serial.print(pin);
    if(pin==21||pin==20){digitalWrite(RELEPIN,HIGH);
      delay(20);
      pinValue=analogRead(pin);
      digitalWrite(RELEPIN,LOW);delay(200);
      }
    else{pinMode(pin,INPUT_PULLUP);
      delay(1);
      pinValue=analogRead(pin);
      pinMode(pin,INPUT);
      }
    if(debugger ==0){Serial.print("  295  pinValue = ");Serial.println(pinValue);}
    return pinValue;
}

void signals(bool scanflag,byte _level){static uint32_t prevtimelong;
      
  if (millis() - prevtimelong < 1000)
    {
      if(scanflag==LOW)pixels.setPixelColor(0, pixels.Color(0, 20, 0));
      else pixels.setPixelColor(0, pixels.Color(0, 0, 20));
    }
  else if(millis() - prevtimelong >= 1000 && millis() - prevtimelong < 4000 )
    {
      if(_level == 0){pixels.setPixelColor(0, pixels.Color(0, 0, 0));}
      else pixels.setPixelColor(0, pixels.Color(0, 20, 0));
    }
  else prevtimelong = millis();
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

  //if(debugger ==1){Serial.print(level);Serial.print("   ");
  //Serial.print(direction);Serial.print("   ");Serial.println(millis());
  }
}

void debugInfoSetup()
{
  printf_begin();
  radio.printDetails();
  if(debugger ==1){
  Serial.print("Is chip connected" );Serial.println(radio.isChipConnected());
  Serial.print("Failure detected" );Serial.println(radio.failureDetected);}
}

