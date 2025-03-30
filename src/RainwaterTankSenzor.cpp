#include "nRF24L01.h"
#include <Adafruit_NeoPixel.h>
#include <RF24.h>
#include <SPI.h>
#include <arduino.h>
#include <printf.h>

// Node type definitions (must match DeviceType enum in GatewayMain.cpp)
enum DeviceType : uint8_t {
  DEVICE_GATEWAY = 0,
  DEVICE_RAINTANK = 1,
  DEVICE_SWITCHBOX = 2,
  DEVICE_SOUTHGARDEN = 3,
  DEVICE_WESTGARDEN = 4,
  DEVICE_PUMPROOM = 5
};

// Define this node's type
#define THIS_NODE DEVICE_RAINTANK

/******* Set up nRF24L01 radio on SPI bus plus pins 7 & 8 *************/
RF24 radio(9, 10); // 0,3

// Define the addresses for all nodes in the network
// Using 6-character addresses for each node
const byte nodeAddresses[][6] = {
    "GATEW", // Gateway
    "RAITN", // Rain Tank
    "SWTCH", // Switchbox
    "SGRDN", // South Garden
    "WGRDN", // West Garden
    "PUMPR"  // Pump Room
};

// Define node names for debugging and display
const char *nodeNames[] = {"gateway",     "rain tank",  "switchbox",
                           "southgarden", "westgarden", "pump room"};

#define SCANTIME 60000 // ako casto sa ma zistovat hladina
#define DISPLAYPIN 2
#define RELEPIN 3
#define NUMPIXELS 8 // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, DISPLAYPIN, NEO_GRB);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

// USING THE EXACT PACKET STRUCTURE FROM GATEWAYMAIN.CPP
// But with clearer field names for this specific node
struct __attribute__((packed)) dataStruct {
  DeviceType deviceType; // 1 byte - identifies the sending device
  uint16_t waterLevel;   // For rain tank, this stores water level (using
                         // temperature field)
  uint16_t unused1;      // Unused in rain tank (humidity field in gateway)
  uint16_t unused2;      // Unused in rain tank (timeParam1 field in gateway)
  uint16_t unused3;      // Unused in rain tank (timeParam2 field in gateway)
  uint16_t timeStamp;    // Seconds since boot
  uint16_t unused4;      // Unused in rain tank (co2Level field in gateway)
  uint8_t status;        // Status flags
};

// Function declarations
void neopixel(uint8_t level);
void sendDataToGateway(uint8_t level);
void radiosetup();
void pinSettings();
void debugInfoSetup();
uint8_t levelReader();
void receiver();
int inputReader(byte i);
void signals(bool scanflag, byte _level);
void dataSendTest();

// Global variables
dataStruct outgoingData;
dataStruct incomingData;
bool debugger = 0;

void setup() {
  pinSettings();
  Serial.begin(115200);
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  // Initialize the data structure
  outgoingData.deviceType = THIS_NODE;
  outgoingData.waterLevel = 0; // Will store water level
  outgoingData.unused1 = 0;    // Not used
  outgoingData.unused2 = 0;    // Not used
  outgoingData.unused3 = 0;    // Not used
  outgoingData.timeStamp = 0;  // Will be set when sending
  outgoingData.unused4 = 0;    // Not used
  outgoingData.status = 0;     // No status flags set initially

  radiosetup();

  // Initial level reading
  uint8_t initialLevel = levelReader();
  outgoingData.waterLevel = initialLevel; // Store water level
  neopixel(initialLevel);
}

void loop() {
  static uint8_t level;
  static uint8_t prevlevel;
  static uint64_t prevtime;
  static bool scanflag;

  if (millis() - prevtime > SCANTIME) // hladina vody sa scanuje v case SCANTIME
  {
    level = levelReader();
    outgoingData.waterLevel = level;          // Store water level
    outgoingData.timeStamp = millis() / 1000; // Seconds since boot

    scanflag = HIGH;     // oznamenie scanovania
    prevtime = millis(); // vynulovanie casu

    // Send data to gateway
    sendDataToGateway(level);
  }

  if (level != prevlevel) // pri zmene levelu sa zobrazi zmena
  {
    neopixel(level);
    prevlevel = level; // nastavenie noveho levelu
  }

  signals(scanflag, level); // signaly funkcii (pre scanovanie modra farba,pre
                            // kontrolu loopu blikanie)

  if (millis() - prevtime > 2000)
    scanflag = LOW; // vypnutie oznamenia scanovania

  // Check for incoming messages
  receiver();

  delay(500);
}

void sendDataToGateway(uint8_t level) {
  // Stop listening to prepare for sending
  radio.stopListening();

  // Update timestamp before sending
  outgoingData.timeStamp = millis() / 1000;

  if (debugger == 0) {
    Serial.print("Sending data from ");
    Serial.print(nodeNames[THIS_NODE]);
    Serial.print(" to ");
    Serial.print(nodeNames[DEVICE_GATEWAY]);
    Serial.print(", water level: ");
    Serial.println(level);
  }

  // Send data directly to gateway
  bool ok = radio.write(&outgoingData, sizeof(outgoingData));

  if (ok && debugger == 0) {
    Serial.println("Transmission successful.");
  } else if (debugger == 0) {
    Serial.println("Transmission failed.");
  }

  // Resume listening
  radio.startListening();
}

void radiosetup() {
  SPI.begin();
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setChannel(74);

  // Enable auto-acknowledgment for reliability
  radio.setAutoAck(true);

  // Set retry delay and count
  radio.setRetries(5, 15);

  // Configure addresses - write to gateway, listen on this node's address
  radio.openWritingPipe(
      nodeAddresses[DEVICE_GATEWAY]);                 // Always send to gateway
  radio.openReadingPipe(1, nodeAddresses[THIS_NODE]); // Listen on our address

  // Start in listening mode
  radio.startListening();

  if (debugger == 0) {
    Serial.println("Radio initialized");
    Serial.print("Node type: ");
    Serial.print(THIS_NODE);
    Serial.print(" (");
    Serial.print(nodeNames[THIS_NODE]);
    Serial.println(")");
  }
}

void pinSettings() {
  pinMode(RELEPIN, OUTPUT);    // D3
  pinMode(0, INPUT_PULLUP);    // D0
  pinMode(1, INPUT_PULLUP);    // D1
  pinMode(DISPLAYPIN, OUTPUT); // D2
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP); // reserve output
  pinMode(A0, INPUT);       // A0
  pinMode(A1, INPUT);       // A1
  pinMode(A2, INPUT);       //...
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, INPUT); // A7
  // pinMode(10,INPUT); // reserved for SPI
  // pinMode(11,INPUT);
}

uint8_t levelReader() {
  int volts;
  static uint8_t _level = 0;

  if (debugger == 1) {
    Serial.print("  line 136 _level ");
    Serial.print(_level);
  }

  for (int i = 21; i >= 14; i--) {
    if (debugger == 1) {
      Serial.print(" , i = ");
      Serial.print(i);
    }
    /* pre ochranu rele citaj hodnoty pinov 21 a 20 len ak je ndrz napustena
     * aspon do urvne 6 alebo 7 ****/
    if (i == 21 && ((_level == 8) || (_level == 7))) {
      // volts=inputReader(i);
      if (inputReader(i) <= 850) {
        _level = i - 13;
        break;
      }
    }

    if (i == 20 && ((_level == 7) || (_level == 6))) {
      // volts=inputReader(i);
      if (inputReader(i) <= 850) {
        _level = i - 13;
        break;
      }
    }

    if (i < 20 && i >= 14) {
      volts = inputReader(i);
      if (volts <= 850) {
        _level = i - 13;
        break;
      } else
        _level = 0;
    }
  }
  if (debugger == 1) {
    Serial.print(" , line 159 volts = ");
    Serial.println(volts);
  }

  return _level;
}

int inputReader(byte pin) {
  int pinValue;
  if (debugger == 1) {
    Serial.print(" 242 pin ");
    Serial.print(pin);
  }
  if (pin == 21 || pin == 20) {
    digitalWrite(RELEPIN, HIGH);
    delay(20);
    pinValue = analogRead(pin);
    digitalWrite(RELEPIN, LOW);
    delay(200);
  } else {
    pinMode(pin, INPUT_PULLUP);
    delay(1);
    pinValue = analogRead(pin);
    pinMode(pin, INPUT);
  }
  if (debugger == 1) {
    Serial.print("  177  pinValue = ");
    Serial.println(pinValue);
  }

  return pinValue;
}

void neopixel(uint8_t level) {
  pixels.clear(); // Set all pixel colors to 'off'

  for (int i = 0; i <= level; i++) {
    pixels.setPixelColor(i - 1, pixels.Color(0, 20, 0));
  }

  pixels.show();
}

void receiver() {
  // Check if there is data available
  if (radio.available()) {
    // Read the incoming data
    radio.read(&incomingData, sizeof(incomingData));

    if (debugger == 0) {
      Serial.print("Received packet from node: ");
      Serial.print(incomingData.deviceType);
      if (incomingData.deviceType < 6) {
        Serial.print(" (");
        Serial.print(nodeNames[incomingData.deviceType]);
        Serial.println(")");
      } else {
        Serial.println(" (unknown)");
      }

      Serial.print("Status: ");
      Serial.println(incomingData.status);
    }

    // Process commands if this message is from the gateway
    if (incomingData.deviceType == DEVICE_GATEWAY) {
      // Handle commands based on status field
      switch (incomingData.status) {
      case 1: // Example: Reset device
        // Implement reset logic
        break;

      case 2: // Example: Change reporting interval
        // Could use waterLevel field to store new interval
        // SCANTIME = incomingData.waterLevel;
        break;

        // Add more commands as needed

      default:
        // Unknown command
        break;
      }
    }
  }
}

void signals(bool scanflag, byte _level) {
  static uint32_t prevtimelong;

  if (millis() - prevtimelong < 1000) {
    if (scanflag == LOW)
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
    else
      pixels.setPixelColor(0, pixels.Color(0, 0, 20));
  } else if (millis() - prevtimelong >= 1000 &&
             millis() - prevtimelong < 4000) {
    if (_level == 0) {
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    } else
      pixels.setPixelColor(0, pixels.Color(0, 20, 0));
  } else
    prevtimelong = millis();
  pixels.show();
}

void dataSendTest() {
  radio.stopListening();

  outgoingData.deviceType = THIS_NODE;
  outgoingData.waterLevel = 22; // Test water level
  outgoingData.timeStamp = millis() / 1000;

  Serial.print("Test sending from ");
  Serial.print(nodeNames[THIS_NODE]);
  Serial.print(" to ");
  Serial.print(nodeNames[DEVICE_GATEWAY]);
  Serial.print(", water level: ");
  Serial.println(outgoingData.waterLevel);

  bool ok = radio.write(&outgoingData, sizeof(outgoingData));
  if (ok)
    Serial.println("Test transmission successful.");
  else
    Serial.println("Test transmission failed.");

  radio.startListening();
}

void debugInfoSetup() {
  printf_begin();
  radio.printDetails();
  if (debugger == 1) {
    Serial.print("Is chip connected: ");
    Serial.println(radio.isChipConnected());
    Serial.print("Failure detected: ");
    Serial.println(radio.failureDetected);
  }
}