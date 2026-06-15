/*
  AirDMX Module - Art-Net Node for ESP-01

  Receives Art-Net DMX data over WiFi and sends it out via RS485 (SN75176).

  Hardware:
  - ESP-01: GPIO0 = LED (active LOW via 3.9K to VCC)
             GPIO1 = UART0 TX  -> SN75176 pin 4 (DI)
             GPIO3 = UART0 RX  <- SN75176 pin 1 (RO)  [unused for DMX output]
  - SN75176: DE (pin 3) and RE (pin 2) assumed tied for transmit-only
             (DE=HIGH always, RE=HIGH always)

  First-boot WiFi setup:
    Connect to AP "AirDMX-Setup" (password: airdmx123), browse to 192.168.10.1
    Enter your WiFi credentials — saved to EEPROM for future boots.

  LED behaviour:
    1 blink  = booting
    3 blinks = WiFi connected, Art-Net ready
    Flashes  = DMX frame being sent
*/

#include <ArtnetnodeWifi.h>
#include <WiFiManager.h>

#define DMX_LED_PIN  0    // GPIO0, active LOW

ArtnetnodeWifi artnet;
WiFiManager    wifiManager;

// ---------------------------------------------------------------------------
// Send one DMX frame via UART0 -> SN75176 RS485 transceiver
//
// DMX512 frame structure:
//   BREAK  : TX line held LOW for >= 92 µs  (we use 176 µs)
//   MAB    : TX line HIGH for >= 12 µs
//   Start  : 0x00 start code
//   Data   : up to 512 channel bytes at 250 kbaud, 8N2
// ---------------------------------------------------------------------------
void sendDMX(uint8_t* data, uint16_t length) {
  // --- Generate BREAK ---
  // Take direct control of GPIO1 (UART TX pin) to pull it low
  pinMode(1, OUTPUT);
  digitalWrite(1, LOW);
  delayMicroseconds(176);   // BREAK duration

  // --- Mark After Break (MAB) ---
  digitalWrite(1, HI