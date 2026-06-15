/*
  AirDMX Module - Art-Net Node for ESP-01

  Receives Art-Net DMX data over WiFi and sends it out via RS485 (SN75176).

  Hardware:
  - ESP-01: GPIO0 = LED (active LOW via 3.9K to VCC)
             GPIO1 = UART0 TX -> SN75176 pin 4 (DI)
             GPIO3 = UART0 RX  [unused for DMX output]
  - SN75176: DE pin 3 pulled HIGH (driver always on)
             RE pin 2 pulled HIGH (receiver disabled)

  First-boot WiFi setup:
    Connect to AP "AirDMX-Setup" (password: airdmx123)
    Browse to 192.168.10.1, enter WiFi credentials.
    Saved to EEPROM for future boots.

  LED behaviour:
    1 blink  = booting
    3 blinks = WiFi connected, Art-Net ready
    Flashes  = DMX frame being sent
*/

#include <ArtnetnodeWifi.h>
#include <WiFiManager.h>

#define DMX_LED_PIN 0   // GPIO0, active LOW

ArtnetnodeWifi artnet;
WiFiManager wifiManager;

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
  Serial.write(data, length);
  Serial.flush();
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  // LED off initially (active LOW -> HIGH = off)
  pinMode(DMX_LED_PIN, OUTPUT);
  digitalWrite(DMX_LED_PIN, HIGH);

  // Single blink = booting
  digitalWrite(DMX_LED_PIN, LOW);
  delay(200);
  digitalWrite(DMX_LED_PIN, HIGH);
  delay(200);

  // Serial at 115200 for WiFiManager debug output during setup
  Serial.begin(115200);
  delay(100);

  // Connect to WiFi (captive portal on first boot)
  wifiManager.begin();

  // Switch UART to DMX mode now that WiFi is up
  Serial.begin(250000, SERIAL_8N2);

  // Bind Art-Net UDP socket after WiFi is connected
  artnet.begin("AirDMX");
  artnet.setName("AirDMX Node");
  artnet.setNumPorts(1);
  artnet.setStartingUniverse(0);
  artnet.enableDMXOutput(0);
  artnet.setDMXOutput(0, 0, 0);  // output 0, UART 0, Universe 0

  // Triple blink = ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(DMX_LED_PIN, LOW);
    delay(80);
    digitalWrite(DMX_LED_PIN, HIGH);
    delay(80);
  }
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
  // Reconnect WiFi if dropped
  if (!wifiManager.isConnected()) {
    static unsigned long lastReconnect = 0;
    if (millis() - lastReconnect > 5000) {
      WiFi.reconnect();
      lastReconnect = millis();
    }
    delay(100);
    return;
  }

  // Poll for incoming Art-Net packets
  uint16_t opcode = artnet.read();

  if (opcode == OpDmx) {
    uint8_t* dmxData = artnet.getDmxFrame();

    // Flash LED to show DMX activity
    digitalWrite(DMX_LED_PIN, LOW);   // LED ON
    sendDMX(dmxData, 512);            // Send full 512-channel universe
    digitalWrite(DMX_LED_PIN, HIGH);  // LED OFF
  }
}
