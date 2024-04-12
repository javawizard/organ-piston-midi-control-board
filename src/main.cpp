
// #include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>

#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_MCP23XXX.h>

#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel esp_board_led(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

std::vector<uint8_t> expander_addresses = {
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

std::vector<Adafruit_MCP23X17*> expanders = {};

void setup() {
  delay(500);

  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  esp_board_led.begin();
  esp_board_led.setBrightness(20);

  // for (uint8_t i = 0; i < expander_addresses.size(); i++) {
  //   expanders.push_back(new Adafruit_MCP23X17());
  // }
}

void loop() {
  esp_board_led.fill(0xFF00000);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0xFF00FF);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0x0000FF);
  esp_board_led.show();
  delay(500);
}
