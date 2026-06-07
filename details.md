I have a SmartShow AirDMX that stopped working not long after I got it.

I am trying to use this code to make thae unit work agin.

MCU is an ESP-01 esp8266, the RS485 is done using a SN75716 transciever, there is an LED and a DC-DC converter module

SN75176

pin 1 RO  - Receiver out

pin 2 RE  - Receiver enable (enabled when this pin is LOW)

pin 3 DE  - Driver   enable (enabled when this pin is HIGH)

pin 4 DI  - Driver   in (the transmitter pin)

pin 5 GND - Ground (0V)

pin 6 A   - Connect to pin A of the other 485 IC

pin 7 B   - Connect to pin B of the other 485 IC

pin 8 Vcc - Power (Vcc)


ESP-01

pin 1 GND

pin 2 GPIO1 UOTXD

pin 3 GPIO3

pin 4 Ch_PD Ch_RN

pin 5 GPIO0 SPI_CS2

pin 6 Reset

pin 7 GPIO3 UORXD

pin 8 VCC


DC-DC converter

pin 1 output +

pin 2 common gnd

pin 3 input +

pin 4 Enable, not used.
