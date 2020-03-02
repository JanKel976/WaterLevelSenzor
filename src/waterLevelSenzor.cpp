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
byte addresses[][6] = {"1Node","2Node"};     // Radio pipe addresses for the 2 nodes to communicate.

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

struct dataPak{char source[7];char target[7];char type[7];float value;};
struct dataPak toBeSendedPak;
struct dataPak acknowledgeData;

//struct RecvPak{char source[7];char target[7];char type[7];float value;}acknowledgeData;

void setup() 
{
  unusedPins();
  Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  radiosetup();
  debugInfoSetup();
}

void loop() 
{
static uint8_t level;
//level=levelReader();
level=ledtesting(level);
neopixel(level);
sendingToPumpControl(level);

Serial.print("50 level = ");Serial.println(level);
//neopixel(level);
//delay(DELAYVAL);
delay(1000);
}

void sendingToPumpControl(float level)
{ /****************** Ping Out Role ***************************/  
  strncpy(toBeSendedPak.source,SourcePoint,7);//SourcePoint je nazov tohto pointu urceny v header pomocou #define
  strncpy(toBeSendedPak.target,"P1Cont",7);
  strncpy(toBeSendedPak.type,"wtrlev",7);
//  float  level = levelReader();
    
  toBeSendedPak.value = level;

    radio.stopListening();                                    // First, stop listening so we can talk.
    Serial.println(F("Now sending"));
//    toBeSendedPak._micros = micros();
     if (!radio.write( &toBeSendedPak, sizeof(toBeSendedPak) )){
       Serial.println(F("failed"));
     }
    radio.startListening();                                    // Now, continue listening
    unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not

    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        Serial.println(F("Failed, response timed out."));
    }else{
                                                                // Grab the response, compare, and send to debugging spew
        radio.read( &acknowledgeData, sizeof(acknowledgeData) );
                
        // Spew it
        Serial.print(F("Sent "));
        Serial.print(toBeSendedPak.value);
        Serial.print(F(", Got response source= "));
        Serial.print(acknowledgeData.source);Serial.print("  ");
//        Serial.print(F(", Round-trip delay "));
//        Serial.print(time-toBeSendedPak._micros);
//        Serial.print(F(" microseconds Value "));
        Serial.print(acknowledgeData.target);Serial.print("  ");
        Serial.print(acknowledgeData.type);Serial.print("  ");
        Serial.println(acknowledgeData.value);Serial.print("  ");
 
    }

    // Try again 1s later

}  

void radiosetup()
{
      /*  radiosetup   */
  radio.begin();

 // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  
  
  toBeSendedPak.value = 1.22;
  // Start the radio listening for data
  radio.startListening();                    // Start listening
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
