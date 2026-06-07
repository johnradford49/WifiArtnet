# AirDMX - Art-Net Node for ESP-01

Art-Net to DMX converter running on ESP-01 (ESP8266) with RS485 output.

## Hardware

- **MCU:** ESP-01 (ESP8266)
- **RS485 Transceiver:** SN75176
- **Power:** DC-DC converter
- **Communication:** WiFi Art-Net input, RS485 DMX output

## PlatformIO Setup

1. Install VS Code: https://code.visualstudio.com/
2. Install PlatformIO extension in VS Code
3. Open this folder in VS Code
4. Build: `Ctrl+Shift+B` → Select "PlatformIO: Build"
5. Flash: Connect ESP-01 via USB adapter
6. Upload: `Ctrl+Shift+B` → Select "PlatformIO: Upload"

## Compilation

The project uses PlatformIO for proper multi-file C++ compilation.

Binary location after build: `.pio/build/esp01/firmware.bin`

## Flashing

Using esptool.py:
```bash
python esptool.py --chip esp8266 --port COM3 write_flash 0x00000 .pio/build/esp01/firmware.bin
```

## Configuration

Edit `Artnetnode.ino` to set:
- WiFi SSID and password
- Art-Net universe and port settings
- RS485 communication parameters
