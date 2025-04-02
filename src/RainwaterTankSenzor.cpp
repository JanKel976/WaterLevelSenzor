#include "nRF24L01.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RF24.h>
#include <SPI.h>
#include <printf.h>

// Node type definitions (must match DeviceType enum in GatewayMain.cpp)
enum DeviceType : uint8_t {
  DEVICE_MUSHROOM,
  DEVICE_GREENHOUSE,
  DEVICE_DESTILA,
  DEVICE_TANKDATA, // Changed from DEVICE_DRYROOM to DEVICE_TANKDATA
  DEVICE_STORAGE,
  DEVICE_LABORATORY,
  DEVICE_GROWROOM,
  DEVICE_CHAMBER
};

/******* Set up nRF24L01 radio on SPI bus plus pins 7 & 8 *************/
RF24 radio(9, 10); //

// Define the addresses for all nodes in the network
// Using 6-character addresses for each node


// Node type definitions (separate from device types)

// Define this node's type
const uint8_t RAITN_addr[5] = {0xF1, 0xB6, 0xB5, 0xB4, 0xB3}; // LSB first
const uint8_t GATEW_addr[5] = {0x78, 0x78, 0x78, 0x78, 0x78};

// Define this node's device type (what kind of data it reports)
#define THIS_DEVICE DEVICE_TANKDATA
#define THIS_NODE 1    // This node's ID in the network (0 for gateway)
#define NODE_GATEWAY 0 // Gateway node ID

// Define node names for debugging and display
const char *nodeNames[] = {"gateway",     "rain tank",  "switchbox",
                           "southgarden", "westgarden", "pump room"};

#define SCANTIME 15000 // ako casto sa ma zistovat hladina
#define DISPLAYPIN 2
#define RELEPIN 3
#define NUMPIXELS 8 // Popular NeoPixel ring size

Adafruit_NeoPixel pixels(NUMPIXELS, DISPLAYPIN, NEO_GRB);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

// USING THE EXACT PACKET STRUCTURE FROM GATEWAYMAIN.CPP
// But with clearer field names for this specific node
struct __attribute__((packed)) dataStruct {
  DeviceType deviceType; // 1 byte - identifies the sending device
  uint16_t timeStamp;   // For rain tank, this stores water level (using
  uint16_t waterLevel;   // level 1-8 in rain tank (value1 field in gateway)
  uint16_t unused2;      // Unused in rain tank (value2  field in gateway)
  uint16_t unused3;      // Unused in rain tank (value3  field in gateway)
  uint16_t unused4;      // Unused in rain tank (value4  field in gateway)
  uint16_t unused5;      // Unused in rain tank (value5  field in gateway)
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
  outgoingData.deviceType =
      THIS_DEVICE;             // Changed from THIS_NODE to THIS_DEVICE
  outgoingData.waterLevel = 0; // Will store water level
  outgoingData.unused2 = 0;    // Not used
  outgoingData.unused3 = 0;    // Not used
  outgoingData.unused4 = 0;    // Not used
  outgoingData.timeStamp = 0;  // Will be set when sending
  outgoingData.unused5 = 0;    // Not used
  outgoingData.status = 0;     // No status flags set initially

  radiosetup();

  // Initial level reading
  uint8_t initialLevel = levelReader();
  outgoingData.waterLevel = initialLevel; // Store water level
  neopixel(initialLevel);

  // Send initial data to gateway
  sendDataToGateway(initialLevel);
}

void loop() {
  static uint8_t level;
  static uint8_t prevlevel;
  static uint64_t prevtime;
  static bool scanflag;

  // Periodically read water level and send data to gateway
  if (millis() - prevtime > SCANTIME) {
    level = levelReader();
    outgoingData.waterLevel = level;          // Store water level
    outgoingData.timeStamp = millis() / 1000; // Seconds since boot

    scanflag = HIGH;     // oznamenie scanovania
    prevtime = millis(); // vynulovanie casu

    // Send data to gateway and receive any commands in the ACK payload
    sendDataToGateway(level);
    // dataSendTest();
    //  After sending, we can go to sleep or low power mode
    //  enterLowPowerMode(); // Uncomment if implementing sleep mode
  }

  // Update display if level changed
  if (level != prevlevel) {
    neopixel(level);
    prevlevel = level; // nastavenie noveho levelu
  }

  // Visual indicators
  signals(scanflag, level);

  // Reset scan flag after 2 seconds
  if (millis() - prevtime > 2000)
    scanflag = LOW; // vypnutie oznamenia scanovania

  // No longer need to check for incoming commands here
  // receiver(); // Remove this line

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
    Serial.print(nodeNames[0]); // Gateway is index 0
    Serial.print(", water level: ");
    Serial.println(level);
  }

  // Send data directly to gateway and wait for acknowledgment
  bool ok = radio.write(&outgoingData, sizeof(outgoingData));

  if (ok) {
    if (debugger == 0) {
      Serial.println("Transmission successful, ACK received.");
    }

    // Check if there's an acknowledgment payload
    if (radio.isAckPayloadAvailable()) {
      // Read the acknowledgment payload
      radio.read(&incomingData, sizeof(incomingData));

      if (debugger == 0) {
        Serial.print("Received command in ACK payload, status: ");
        Serial.println(incomingData.status);
      }

      // Process commands based on status field
      switch (incomingData.status) {
      case 1: // Request for immediate water level reading
      {
        // We just sent a reading, no need to send again
        // Just acknowledge we received the command
        if (debugger == 0) {
          Serial.println("Acknowledged request for water level");
        }
      } break;

      case 3: // Reset device
      {
        Serial.println("Resetting device...");
        delay(100);
        // Software reset
        asm volatile("jmp 0");
      } break;

      default:
        // Unknown command
        if (debugger == 0) {
          Serial.print("Unknown command: ");
          Serial.println(incomingData.status);
        }
        break;
      }
    }
  } else if (debugger == 0) {
    Serial.println("Transmission failed, no ACK received.");
  }

  // Resume listening for any other communications
  radio.startListening();
}

void radiosetup() {
  bool hardwareConnected;
  SPI.begin();
  hardwareConnected = radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setChannel(74);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.setCRCLength(RF24_CRC_16);

  // Enable auto-acknowledgment for reliability
  radio.setAutoAck(true);

  // Set retry delay and count
  radio.setRetries(15, 15);

  // Configure addresses - write to gateway, listen on this node's address
  radio.openWritingPipe(RAITN_addr);   // Always send to gateway
  radio.openReadingPipe(1, GATEW_addr); // Listen on our address

  // set to transmitting mode

  printf_begin();
  if (debugger == 0 && hardwareConnected) {
    Serial.println("Radio initialized");
    Serial.print("Node type: ");
    Serial.print(THIS_NODE);
    Serial.print(" (");
    Serial.print(nodeNames[THIS_NODE]);
    Serial.println(")");
    Serial.print("Reporting device type: ");
    Serial.print(THIS_DEVICE);
    Serial.println(" (tank data)");
  }
  if (debugger == 0 && !hardwareConnected) {
    Serial.println("Hardware not connected");
  }

  radio.printPrettyDetails();
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

  outgoingData.deviceType = DEVICE_TANKDATA;
  outgoingData.waterLevel = 22; // Test water level
  outgoingData.timeStamp = millis() / 1000;

  Serial.print("Test sending from ");
  Serial.print(nodeNames[THIS_NODE]);
  Serial.print(" to ");
  Serial.print(nodeNames[NODE_GATEWAY]);
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
