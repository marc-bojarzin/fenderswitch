#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#include "Task.hpp"
#include "Button.hpp"
#include "Led.hpp"
#include "RotaryEncoder.hpp"
#include "LcdBuffer.hpp"
#include "OpticalRelay.hpp"
#include "StatsTask.hpp"
#include "EncoderTask.hpp"
#include "PresetLoader.hpp"
#include "ChannelTask.hpp"

// Sandbox: everything runs as cooperative tasks, without delay().
//  - StatsTask prints how often loop() ran per second, proving that nothing
//    blocks. Off by default; turn it on with -DSTATS_INTERVAL_S=10.
//  - PresetLoader loads all presets from EEPROM at startup and selects
//    program 1.
//  - EncoderTask browses the presets with a rotary encoder (bare, no
//    pull-ups: A/CLK on pin 2, B/DT on pin 4, SW on pin 5, C and the other SW
//    pin to GND): each step selects
//    the next or previous program and switches the amp to its channel through
//    the OpticalRelay. The program is shown on a 16x2 I2C LCD (SDA on A4, SCL
//    on A5, address LCD_I2C_ADDR). Pressing "enter" toggles the (still empty)
//    menu mode. The "Store" button (A3, to GND) stores a dirty preset.
//  - ChannelTask reads the Clean, Drive and More Drive buttons (A0-A2, to
//    GND): a press switches the amp to that channel, the LED swells while the
//    preset differs from the stored one, holding the button 2 s stores it.
//  - Red, green and yellow on/off LEDs (each via 220-330 ohm resistor to GND)
//    show the selected channel. On the prototype, OUT_CH and OUT_GAIN drive
//    plain LEDs (220-330 ohm to GND) instead of the relay stages: lit = relay
//    ON.

// Pin assignments.
#define ENC_CLK_PIN     2  // Rotary encoder CLK pin (clock signal)
#define ENC_DT_PIN      4  // Rotary encoder DT pin (data)
#define ENC_SW_PIN      5  // Rotary encoder SW pin (push button)
#define OUT_CH_PIN      7  // Relay 1 (channel), HIGH = relay ON
#define OUT_GAIN_PIN    8  // Relay 2 (gain), HIGH = relay ON
#define RED_LED_PIN     9  // Red LED pin ("More Drive")
#define YELLOW_LED_PIN 10  // Yellow LED pin ("Drive")
#define GREEN_LED_PIN  11  // Green LED pin ("Clean")
#define BTN_CLEAN_PIN  A0  // "Clean" button, to GND (INPUT_PULLUP)
#define BTN_DRIVE_PIN  A1  // "Drive" button, to GND (INPUT_PULLUP)
#define BTN_MORE_PIN   A2  // "More Drive" button, to GND (INPUT_PULLUP)
#define BTN_STORE_PIN  A3  // "Store" button next to the encoder, to GND

// LCD on I2C, default address 0x27 (A0-A2 not jumpered).
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

// Optical relay instance: channel and gain pins specified above.
static OpticalRelay relay{OUT_CH_PIN, OUT_GAIN_PIN};

// All presets in RAM plus the working buffer, loaded from EEPROM in setup().
static PresetLoader presets;

static LiquidCrystal_I2C lcd{LCD_I2C_ADDR, LCD_COLS, LCD_ROWS};
static LcdBuffer lcd_buffer{&lcd};

// Interrupt service routine for the rotary encoder CLK pin. Calls the encoder's
// on_clk_change() method.
static void onEncoderClk() { encoder.on_clk_change(); }

// Stats task instance: keeps track of various statistics.
static StatsTask stats;

// Encoder task instance: handles the rotary encoder and related buttons, updates the LCD and LEDs accordingly.
static EncoderTask encoderTask {
    encoder, store_button, presets,
    relay,lcd_buffer,
    led_green, led_yellow, led_red
};

// Channel task instance: the three channel buttons, displayed through
// encoderTask.
static ChannelTask channelTask {
    clean_button, drive_button, more_button, presets, encoderTask
};

// Array of all tasks to be run in the main loop.
static Task* tasks[] = {
    &relay,
    &led_red, &led_green, &led_yellow,
    &stats, &encoderTask, &channelTask
};

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println(F("fenderswitch started"));
    lcd.init();
    lcd.backlight();
    lcd_buffer.update(0, "fenderswitch    ");
    presets.begin();
    encoderTask.apply();
    attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), onEncoderClk, CHANGE);
}

void loop() {
    stats.count();
    runTasks(tasks);
}
