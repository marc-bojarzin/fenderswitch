#include <Arduino.h>
#include "Task.hpp"
#include "Button.hpp"
#include "Led.hpp"
#include "RotaryEncoder.hpp"
#include "SegmentDisplay.hpp"
#include "Relay.hpp"
#include "StatsTask.hpp"
#include "EncoderTask.hpp"
#include "PresetLoader.hpp"
#include "ChannelTask.hpp"
#include "MidiTask.hpp"
#include "Settings.hpp"

// Sandbox: everything runs as cooperative tasks, without delay().
//  - StatsTask prints how often loop() ran per second, proving that nothing
//    blocks. Off by default; turn it on with -DSTATS_INTERVAL_S=10.
//  - PresetLoader loads all presets from EEPROM at startup and selects
//    program 1.
//  - EncoderTask browses the presets with a rotary encoder (bare, 10k
//    pull-ups on A, B and SW: A/CLK on pin 2, B/DT on pin 4, SW on pin 5, C
//    and the other SW pin to GND): each step selects
//    the next or previous program and switches the amp to its channel through
//    the Relay. The program number is shown on a 4-digit TM1637
//    7-segment display (DIO on A4, CLK on A5). Pressing "enter" opens the
//    settings menu (MIDI channel, display brightness). The "Store" button (A3, to GND)
//    stores a dirty preset, or confirms the menu entry.
//  - ChannelTask reads the Clean, Drive and More Drive buttons (A0-A2, to
//    GND): a press switches the amp to that channel, the LED swells while the
//    preset differs from the stored one, holding the button 2 s stores it.
//  - MidiTask reads MIDI IN on the UART (D0/RX, from the DIN socket or from
//    a serial-MIDI bridge over USB): a Program Change recalls its preset.
//    Listens to the channel in Settings (EEPROM), or to all (OMNI).
//  - Red, green and yellow on/off LEDs (each via 220-330 ohm resistor to GND)
//    show the selected channel.
//  - OUT_CH and OUT_GAIN drive the relay coils through BC548C transistors
//    (1k to the base, 10k base pull-down), the amp line uses the NC contacts.
//    On the prototype, plain LEDs (220-330 ohm to GND) take their place:
//    lit = coil energized = contact open.

// Pin assignments.
#define ENC_CLK_PIN     2  // Rotary encoder CLK pin (clock signal)
#define ENC_DT_PIN      4  // Rotary encoder DT pin (data)
#define ENC_SW_PIN      5  // Rotary encoder SW pin (push button)
#define OUT_CH_PIN      7  // Relay 1 driver (channel), HIGH = contact open
#define OUT_GAIN_PIN    8  // Relay 2 driver (gain), HIGH = contact open
#define RED_LED_PIN     9  // Red LED pin ("More Drive")
#define YELLOW_LED_PIN 10  // Yellow LED pin ("Drive")
#define GREEN_LED_PIN  11  // Green LED pin ("Clean")
#define BTN_CLEAN_PIN  A0  // "Clean" button, to GND (INPUT_PULLUP)
#define BTN_DRIVE_PIN  A1  // "Drive" button, to GND (INPUT_PULLUP)
#define BTN_MORE_PIN   A2  // "More Drive" button, to GND (INPUT_PULLUP)
#define BTN_STORE_PIN  A3  // "Store" button next to the encoder, to GND
#define SEG_DIO_PIN    A4  // 7-segment display DIO (was LCD SDA)
#define SEG_CLK_PIN    A5  // 7-segment display CLK (was LCD SCL)

// LCD on I2C, default address 0x27 (A0-A2 not jumpered). Not used at the
// moment: the 7-segment display took its pins (A4/A5). LcdBuffer is still in
// the project; an LCD needs Wire on A4/A5, which the TM1637 can't share.
#define LCD_I2C_ADDR 0x27

// UART speed: the MIDI baud rate. MIDI IN uses RX, Serial.print() uses TX, so
// the serial monitor has to run at the same rate (monitor_speed).
#define SERIAL_BAUD 31250

// LED instances: red, green and yellow LEDs specified above.
static Led led_red{RED_LED_PIN};
static Led led_green{GREEN_LED_PIN};
static Led led_yellow{YELLOW_LED_PIN};

// Rotary encoder instance: CLK, DT and SW pins specified above.
static RotaryEncoder encoder{ENC_CLK_PIN, ENC_DT_PIN, ENC_SW_PIN};

// "Store" button: stores a dirty preset, or confirms settings in menu mode.
static Button store_button{BTN_STORE_PIN};

// Channel buttons: Clean, Drive and More Drive.
static Button clean_button{BTN_CLEAN_PIN};
static Button drive_button{BTN_DRIVE_PIN};
static Button more_button{BTN_MORE_PIN};

// Relay instance: channel and gain pins specified above.
static Relay relay{OUT_CH_PIN, OUT_GAIN_PIN};

// All presets in RAM plus the working buffer, loaded from EEPROM in setup().
static PresetLoader presets;

// Settings made in the menu (MIDI channel or OMNI, display brightness),
// loaded from EEPROM in setup().
static Settings settings;

// 4-digit 7-segment display with colon (TM1637): shows the program number.
static SegmentDisplay display{SEG_CLK_PIN, SEG_DIO_PIN};

// The encoder needs an interrupt on both CLK and DT, but on the Nano only pins
// 2 and 3 have external interrupts. So it uses the pin-change interrupt of
// port D (PCINT2, pins 0-7), enabled for the two encoder pins only. Both pins
// must be on port D, otherwise the ISR below doesn't fire for them.
static void enablePinChangeInterrupt(uint8_t pin)
{
    *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));
    PCIFR = bit(digitalPinToPCICRbit(pin));  // drop a stale pending flag
    *digitalPinToPCICR(pin) |= bit(digitalPinToPCICRbit(pin));
}

// Pin-change interrupt for port D: an edge on CLK or DT.
ISR(PCINT2_vect) { encoder.on_change(); }

// Stats task instance: keeps track of various statistics.
static StatsTask stats;

// Encoder task instance: handles the rotary encoder and related buttons,
// updates the display and LEDs accordingly.
static EncoderTask encoderTask {
    encoder, store_button, presets,
    relay, display, settings,
    led_green, led_yellow, led_red
};

// Channel task instance: the three channel buttons, displayed through
// encoderTask.
static ChannelTask channelTask {
    clean_button, drive_button, more_button, presets, encoderTask
};

// MIDI task instance: Program Change from MIDI IN, applied through
// encoderTask.
static MidiTask midiTask{encoderTask, settings};

// Array of all tasks to be run in the main loop.
static Task* tasks[] = {
    &relay,
    &led_red, &led_green, &led_yellow,
    &stats, &midiTask, &encoderTask, &channelTask
};

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println(F("fenderswitch started"));
    settings.begin();
    display.begin(settings.brightness());
    presets.begin();
    encoder.begin();
    store_button.begin();
    clean_button.begin();
    drive_button.begin();
    more_button.begin();
    if (settings.midi() == MIDI_OMNI_SETTING)
    {
        Serial.println(F("midi: omni"));
    }
    else
    {
        Serial.print(F("midi: channel="));
        Serial.println(settings.midi());
    }
    Serial.print(F("display: brightness="));
    Serial.println(settings.brightness() + 1);
    encoderTask.apply();
    enablePinChangeInterrupt(ENC_CLK_PIN);
    enablePinChangeInterrupt(ENC_DT_PIN);
}

void loop() {
    stats.count();
    runTasks(tasks);
}
