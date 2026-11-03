#ifndef ENCODER_TASK_HPP
#define ENCODER_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "Button.hpp"
#include "AmpChannel.hpp"
#include "Led.hpp"
#include "RotaryEncoder.hpp"
#include "LcdBuffer.hpp"
#include "OpticalRelay.hpp"
#include "PresetLoader.hpp"

// How long the encoder has to rest before the amp switches to the browsed
// program, in ms. Browsing past programs only updates the LCD, so the amp
// doesn't jump through every channel on the way.
#ifndef BROWSE_SETTLE_MS
#define BROWSE_SETTLE_MS 300
#endif

// Browses the presets with the rotary encoder.
//
// Every step selects the next (+1) or previous (-1) program, wrapping around
// between the first and the last. Selecting drops an unsaved change in the
// working buffer and shows the program on the second LCD line right away. Once
// the encoder has rested for BROWSE_SETTLE_MS, the amp switches to the preset's
// channel and its LED lights up.
//
// Pressing "Enter" toggles menu mode, where turning doesn't change the program.
// The menu itself (MIDI channel, OMNI) is still to come.
//
// The "Store" button next to the encoder stores a dirty working buffer in live
// mode, as an alternative to holding the channel button for 2 s. In menu mode
// it will confirm the MIDI settings and write them to EEPROM.
//
// It also owns the display of the working buffer, used by ChannelTask too: the
// LED of the active channel is steady while the preset is clean and swells
// while it is dirty, and the LCD marks a dirty preset with a "*".
//
// Polls every pass: moved() only reports -1, 0 or +1 per call, so polling
// rarely would lose steps.
class EncoderTask : public Task
{
    RotaryEncoder& encoder_;
    Button& store_;
    PresetLoader& presets_;
    OpticalRelay& relay_;
    LcdBuffer& lcd_buffer_;

    // Channel LEDs: Clean, Drive and More Drive.
    Led& led_green_;
    Led& led_yellow_;
    Led& led_red_;

    // True while in menu mode.
    bool menu_ = false;

    // True while a browsed program waits to be applied.
    bool apply_pending_ = false;
    // millis() of the last encoder step.
    uint32_t moved_at_ = 0;

public:
    // Not default constructible.
    EncoderTask() = delete;

    // Constructor.
    EncoderTask(RotaryEncoder& encoder, Button& store, PresetLoader& presets,
                OpticalRelay& relay, LcdBuffer& lcd_buffer,
                Led& led_green, Led& led_yellow, Led& led_red);

    // Polls the encoder, its button and the Store button; called by
    // runTasks().
    void run() override;

    // Switches the amp to the working buffer's channel and shows it on the
    // LEDs, Serial and the LCD. Cancels a pending delayed apply.
    void apply();

    // Stores a dirty working buffer in its preset and EEPROM and shows it as
    // clean again. Does nothing unless the preset is dirty.
    void store();

private:
    // Lights the LED of the working buffer's channel only: steady while clean,
    // swelling while dirty.
    void showChannel();

    // Writes the program (1-based, as on MIDI gear), its channel and a "*"
    // while dirty, or the menu placeholder, to the second LCD line.
    void show();
};

#endif // ENCODER_TASK_HPP
