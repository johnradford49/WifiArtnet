/*
  AirDMX Module - Art-Net Node for ESP-01

  Receives Art-Net DMX data over WiFi and sends it out via RS485 (SN75176).

  Hardware:
  - ESP-01: GPIO0 = LED (active LOW via 3.9K to VCC)
             GPIO1 = UART0 TX -> SN75176 pin 4 (DI)
             GPIO3 = UART0 RX  [unused for DMX output]
  - SN75176: DE pin 3 pulled HIGH (driver always on)
             RE pin 2 pulled HIGH (receiver disabled)

  WiFi credentials are stored in src/wifi_credentials.h (excluded from git).

  LED behaviour:
    Slow blink (3x) = connecting to WiFi
    Fast blink (3x) = WiFi connected, Art-Net ready
    Flashes         = DMX frame being sent
*/

#include <ESP8266WiFi.h>
#include <ArtnetnodeWifi.h>
#include "wifi_credentials.h"

#define DMX_LED_PIN 0   // GPIO0, active LOW

ArtnetnodeWifi artnet;

// ---------------------------------------------------------------------------
// Send one DMX frame via UART0 -> SN75176 RS485 transceiver
//
// DMX512 frame:
//   BREAK : TX line LOW for >= 92 us  (we use 176 us)
//   MAB   : TX line HIGH for >= 12 us
//   Byte 0: 0x00 start code
//   Data  : up to 512 channel bytes at 250 kbaud, 8N2
// ---------------------------------------------------------------------------
void sendDMX(uint8_t* data, uint16_t length) {
  // Generate BREAK: take direct control of GPIO1 (UART TX pin)
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
  delayMicroseconds(176);

  // Mark After Break
  digitalWrite(1, HIGH);
  delayMicroseconds(12);

  // Restore UART and send DMX frame
  Serial.begin(250000, SERIAL_8N2);
  Serial.write((uint8_t)0x00);  // DMX start code
  Serial.write(data, length