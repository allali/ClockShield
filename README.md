# ClockShield
Code that implements clock with alarm on an Arduino Uno with Clock Shield

The code is designed as a state machine using function pointer. Each state is a function, return value is used for transition implented in the loop.

# Material

To run this code you need an arduino Uno and a clock shield (OPEN SMART Clock Shield v1.1). The clock shield provide:
- realtime clock based on DS1307
- 4 leds (2 red, 1 green, 1 blue)
- 3 buttons
- 7 segments display drived by TM1636
- buzzer
- light sensor
- temperature sensor

# Dependencies

Following arduino libraries are used by the code:
- [RTCLib by Adafruit](https://github.com/adafruit/RTClib) (v2.1.3)
- [Buzzer by Giuseppe Martini](https://github.com/gmarty2000-ARDUINO/arduino-BUZZER/) (v1.0.0)
- TM1637 TODO: provide link
They need the following dependencies:
- [Adafruit Bus IO](https://github.com/adafruit/Adafruit_BusIO)

The code for dealing with TM1636 is included and had been developped by Loovee.

# Author
Julien Allali allali@enseirb-matmeca.fr
