
// #include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>

#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_MCP23XXX.h>

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel esp_board_led(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

uint8_t expander_addresses[] = {
  0x20, // choir 1
  0x21, // choir 2
  0x22, // great 1
  0x23, // great 2
  0x24, // swell 1
  0x25, // swell 2
  0x26, // solo 1
  0x27, // solo 2
  0x60, // pedal 1 (translated through an LTC4316 on the pedal controller)
  0x61  // pedal 2 (translated through an LTC4316 on the pedal controller)
};

Adafruit_MCP23X17 expanders[std::size(expander_addresses)];

void setup() {
  delay(1000);

  Serial.begin(115200);

  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  esp_board_led.begin();
  esp_board_led.setBrightness(20);

  for (size_t i = 0; i < 0 /*std::size(expanders)*/; i++) {
    // TODO: try to re-setup on every loop() to allow hot swapping
    // (Also see if it ever fails to come up and, if it does, do that for the sake of robustness as well)
    if (!expanders[i].begin_I2C(expander_addresses[i])) {
      // Setup failed; flash to indicate which expander can't be found, then reboot and try again.
      for (size_t flash = 0; flash <= i; flash++) {
        esp_board_led.fill(0xFF0000);
        esp_board_led.show();
        delay(250);
        esp_board_led.fill(0);
        esp_board_led.show();
        delay(250);
      }
      delay(1000);
      ESP.restart();
    }
    for (uint8_t pin = 0; pin < 16; pin++) {
      // Honestly this has me tempted to skip using Adafruit_MCP23017 and just write I2C directly; then we could bulk
      // set all of the pins' I/O directions instead of setting them one at a time
      expanders[i].pinMode(pin, INPUT); // the boards I made have external pullup resistors
    }
  }

  esp_board_led.fill(0x00FF00);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0);
  esp_board_led.show();
  delay(500);
}

void loop() {
  Serial.println("Did the thing");

  esp_board_led.fill(0xFF00000);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0xFF00FF);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0x00FFFF);
  esp_board_led.show();
  delay(500);
}
