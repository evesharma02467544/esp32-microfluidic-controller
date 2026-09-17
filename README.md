## Hardware Pinout Directory
```text
ESP32 Dev Module Pinout Diagram
                             +-----------------+
                         3V3 | [ ]         [ ] | GND
             (EN Isolated)   | [ ]         [ ] | GPIO 23 (SPI MOSI - MAX3421E)
                          VP | [ ]         [ ] | GPIO 22 (SERVO_PIN - MG90S Signal)
                          VN | [ ]         [ ] | GPIO 1  (TX0 - Keep Clear)
                     GPIO 34 | [ ]         [ ] | GPIO 3  (RX0 - Keep Clear)
                     GPIO 35 | [ ]         [ ] | GPIO 19 (SPI MISO - MAX3421E)
   (PUMP1_IN1)       GPIO 32 | [ ]         [ ] | GPIO 18 (SPI SCK  - MAX3421E)
   (PUMP1_IN2)       GPIO 33 | [ ]         [ ] | GPIO 5  (SPI SS   - MAX3421E)
   (PUMP2_IN3)       GPIO 25 | [ ]         [ ] | GPIO 17
   (PUMP2_IN4)       GPIO 26 | [ ]         [ ] | GPIO 16
                     GPIO 27 | [ ]         [ ] | GPIO 4  (BUTTON_PIN - Internal Pullup)
   (OLED SDA)        GPIO 13 | [ ]         [ ] | GPIO 0
   (OLED SCL)        GPIO 14 | [ ]         [ ] | GPIO 2
                         GND | [ ]         [ ] | GPIO 15
     (6V Battery Input)   5V | [ ]         [ ] | GND (Common Power Rail Ground)
                             +-----------------+
