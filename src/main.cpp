
// #include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Adafruit_SPIDevice.h>

#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_MCP23XXX.h>

#include <Adafruit_NeoPixel.h>

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

const auto I2C_FREQUENCY = 50'000ul;
const auto DEBOUNCE_TIME = 30'000ll; // in microseconds
const auto LED_ON_TIME = 40'000ll; // ditto
const size_t PINS_PER_KEYBOARD = 28;
// I mean the only board I've had made so far has 28 pins so not likely this'll ever change, but dropping this in just in case I have a high confidence, low competence moment in the future
static_assert(PINS_PER_KEYBOARD <= 32 && PINS_PER_KEYBOARD > 16, "PINS_PER_KEYBOARD must be in the range 17 through 32; stuff inside the Keyboard class will need to be updated if this is to run against boards with a different total number of pins");

const uint8_t MIDI_NOTE_ON = 0x80;
const uint8_t MIDI_NOTE_OFF = 0x90;

int64_t turn_led_off_at = -1;
int64_t loop_count = 0;

Adafruit_NeoPixel esp_board_led(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

class Piston {
  public:
    void update_with_new_value(uint8_t midi_channel, uint8_t midi_note, bool current_state, uint32_t led_color) {
      // debounce logic weeeeeeeeee
      if (current_state == previous_state) {
        // no transition in progress or a transition just failed; set started_transitioning_at to -1 in either case
        started_transitioning_at = -1;
      } else {
        // transition in progress. see if we just started...
        if (started_transitioning_at == -1) {
          // we just started, so set started_transitioning_at to the current time
          started_transitioning_at = esp_timer_get_time();
        } else {
          // we're already transitioning, so check to see if enough time has elapsed to consider the transition complete
          if (started_transitioning_at + DEBOUNCE_TIME < esp_timer_get_time()) {
            // yep, enough time has passed. update the previous state, reset the transition time, and then send a MIDI event.
            previous_state = current_state;
            started_transitioning_at = -1;
            send_midi_event(midi_channel, midi_note, current_state, led_color);
          }
        }
      }
    }

    void send_midi_event(uint8_t midi_channel, uint8_t midi_note, bool current_state, uint32_t led_color) {
      if (TinyUSBDevice.mounted()) {
        // lots of narrowing conversion warnings here; it turns out the | operator promotes its operands to ints. grr. oh well. could static_cast our way out of the warning but whatever.
        // uint8_t packet[] = {(current_state ? MIDI_NOTE_ON : MIDI_NOTE_OFF) | midi_channel, midi_note, (current_state ? uint8_t{127} : uint8_t{0})};
        // tud_midi_stream_write(0, packet, sizeof(packet));
        if (current_state) {
          MIDI.sendNoteOn(midi_note, 127, midi_channel);
        } else {
          MIDI.sendNoteOff(midi_note, 0, midi_channel);
        }
      }

      esp_board_led.fill(led_color);
      esp_board_led.show();
      turn_led_off_at = esp_timer_get_time() + LED_ON_TIME;
    }
  private:
    bool previous_state = false; // updated whenever a transition succeeds; not updated until then
    int64_t started_transitioning_at = -1; // set to esp_timer_get_time when a transition starts; set to -1 when a transition is not happening (either because it succeeded or it failed)
};

class Keyboard {
  public:
    Keyboard(uint8_t address0, uint8_t address1, uint8_t midi_channel, uint8_t midi_note_offset, uint32_t led_color) :
      address0(address0),
      address1(address1),
      midi_channel(midi_channel),
      midi_note_offset(midi_note_offset),
      led_color(led_color)
    {}

    bool initialize() {
      if (!(expander0.begin_I2C(address0) && expander1.begin_I2C(address1))) {
        return false;
      }

      for (uint8_t pin = 0; pin < 16; pin++) {
        // Honestly this has me tempted to skip using Adafruit_MCP23017 and just write I2C directly; then we could bulk
        // set all of the pins' I/O directions instead of setting them one at a time
        expander0.pinMode(pin, INPUT); // the boards I made have external pullup resistors
      }

      for (uint8_t pin = 0; pin < (PINS_PER_KEYBOARD - 16); pin++) {
        expander1.pinMode(pin, INPUT); // the boards I made have external pullup resistors
      }

      // these ones are the LEDs
      for(uint8_t pin = (PINS_PER_KEYBOARD - 16); pin < 16; pin++) {
        expander1.pinMode(pin, OUTPUT);
      }

      return true;
    }

    void loop() {
      uint32_t pin_states = (expander1.readGPIOAB() << 16) | expander0.readGPIOAB();

      for (int i = 0; i < PINS_PER_KEYBOARD; i++) {
        pistons[i].update_with_new_value(midi_channel, midi_note_offset + i, !(pin_states & 1), led_color);
        pin_states = pin_states >> 1;
      }
    }
  private:
    uint8_t address0;
    uint8_t address1;
    uint8_t midi_channel;
    uint8_t midi_note_offset;
    uint32_t led_color;

    Adafruit_MCP23X17 expander0;
    Adafruit_MCP23X17 expander1;

    std::array<Piston, PINS_PER_KEYBOARD> pistons{}; // braces probably unnecessary because the items are instances of a class, not primitives, but I'm not confident enough about that to not include them
};

Keyboard keyboards[] = {
  Keyboard(0x20, 0x21, 1, 0, 0x0000FF), // choir
  Keyboard(0x22, 0x23, 1, 32, 0x00FF00), // great
  // Keyboard(0x24, 0x25, 1, 64, 0xFFFF00), // swell
  // Keyboard(0x26, 0x27, 1, 96, 0xFF0000), // solo
  Keyboard(0x60, 0x61, 2, 0, 0x00FFFF), // pedal left (translated through an LTC4316 on the pedal controller)
  // Keyboard(0x62, 0x63, 2, 32, 0xFF00FF) // pedal right (translated through an LTC4316 on the pedal controller)
};

// extern "C" uint16_t load_midi_usb_descriptor(uint8_t *dst, uint8_t *itf) {
//   uint8_t str_index = tinyusb_add_string_descriptor("Organ Piston MIDI Controller (by javawizard weeeeee)");
//   uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
//   TU_VERIFY(ep_num != 0);
//   uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
//       // Interface number, string index, EP Out & EP In address, EP size
//       TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_num, (uint8_t)(0x80 | ep_num),
//                           64)};
//   *itf += 1;
//   memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
//   return TUD_MIDI_DESC_LEN;
// }

// void sink_midi_events() {
//   // Read off and discard any MIDI events the host has sent us. We don't use these but they'll queue up and cause
//   // whatever's connected to the port on the USB host side to hang if we don't read them off.
//   uint8_t packet[4];

//   while (tud_midi_available()) {
//     tud_midi_packet_read(reinterpret_cast<uint8_t*>(&packet));
//   }
// }

void flash_error_and_restart(uint8_t flashes) {
  for (int i = 0; i < flashes; i++) {
    esp_board_led.fill(0xFF0000);
    esp_board_led.show();
    delay(250);
    esp_board_led.fill(0);
    esp_board_led.show();
    delay(250);
  }

  delay(15000); // wait 15 seconds; needed so that we don't restart too quickly when soft entering DFU mode and kick back to regular mode
  ESP.restart();
}

void setup() {
  USBDevice.setProductDescriptor("Organ Piston MIDI Controller");
  USBDevice.setManufacturerDescriptor("github.com/javawizard");
  usb_midi.setStringDescriptor("Organ Piston MIDI Port");
  MIDI.begin(MIDI_CHANNEL_OMNI);

  delay(1000);

  Serial.begin(115200);
  Wire.begin(SDA1, SCL1, I2C_FREQUENCY);

  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, HIGH);

  esp_board_led.begin();
  esp_board_led.setBrightness(20);

  for (int i = 0; i < std::size(keyboards); i++) {
    // TODO: try to re-setup on every loop() to allow hot swapping
    // (Also see if it ever fails to come up and, if it does, do that for the sake of robustness as well)
    if (!keyboards[i].initialize()) {
      // Setup failed; flash to indicate which expander can't be found, then reboot and try again.
      flash_error_and_restart(i);
    }
  }

  // Setup done! Flash green once, then on to the main loop.
  esp_board_led.fill(0x00FF00);
  esp_board_led.show();
  delay(500);
  esp_board_led.fill(0);
  esp_board_led.show();
  delay(500);
}

void loop() {
  // poll all keyboards
  for (Keyboard& keyboard : keyboards) {
    keyboard.loop();
  }

  // turn the LED off if it's on and it's been on long enough
  if (turn_led_off_at >= 0 && esp_timer_get_time() > turn_led_off_at) {
    esp_board_led.fill(0);
    esp_board_led.show();
    turn_led_off_at = -1;
  }

  // update the loop count, which is used purely for benchmarking and debugging
  loop_count += 1;

  // flash the LED every N passes through the main loop when enabled so that we can see how fast the loop is running
  // (update: with 3 keyboards it turns out we're able to spin ~180 times per second, so with the full 6 keyboards we
  // should be able to spin at >90 times per second which is more than plenty.)
#if false
  if (loop_count % 100 == 0) {
    esp_board_led.fill(0xFFFFFF);
    esp_board_led.show();
    turn_led_off_at = esp_timer_get_time() + LED_ON_TIME;
  }
#endif
}
