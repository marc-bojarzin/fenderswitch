#ifndef ENCODER_TASK_HPP
#define ENCODER_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "Button.hpp"
#include "AmpChannel.hpp"
#include "Led.hpp"
#include "RotaryEncoder.hpp"
#include "SegmentDisplay.hpp"
#include "OpticalRelay.hpp"
#include "PresetLoader.hpp"

// How long the encoder has to rest before the amp switches to the browsed
// program, in ms. Browsing past programs only updates the display, so the amp
// doesn't jump through every channel on the way.
#ifndef BROWSE_SETTLE_MS
#define BROWSE_SETTLE_MS 300
#endif

// How long the channel LED goes dark when there is nothing to store, in ms.
#ifndef NOTHING_TO_STORE_MS
#define NOTHING_TO_STORE_MS 150
#endif

// Browses the presets with the rotary encoder.
//
// Every step selects the next (+1) or previous (-1) program, wrapping around
// between the first and the last. Selecting drops an unsaved change in the
// working buffer and shows the program number on the display right away. Once
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
// while it is dirty.
//
// Polls every pass. moved() accumulates the steps between polls, so a slow
// pass doesn't lose any; several steps are applied at once.
class EncoderTask : public Task
{
    RotaryEncoder& encoder_;
    Button& store_;
    PresetLoader& presets_;
    OpticalRelay& relay_;
    SegmentDisplay& display_;

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
                OpticalRelay& relay, SegmentDisplay& display,
                Led& led_green, Led& led_yellow, Led& led_red);

    // Polls the encoder, its button and the Store button; called by
    // runTasks().
    void run() override;

    // Selects program number (0-127) right away, e.g. after a MIDI Program
    // Change: drops an unsaved change, switches the amp without the browse
    // delay and cancels a pending browse.
    void select(uint8_t number);

    // Switches the amp to the working buffer's channel and shows it on the
    // LEDs, Serial and the display. Cancels a pending delayed apply.
    void apply();

    // Stores a dirty working buffer in its preset and EEPROM and shows it as
    // clean again. On a clean preset it only acknowledges the request: the
    // channel LED goes dark for NOTHING_TO_STORE_MS.
    void store();

private:
    // Lights the LED of the working buffer's channel only: steady while clean,
    // swelling while dirty.
    void showChannel();

    // Returns the LED of channel.
    Led& channelLed(AmpChannel channel);

    // Shows the program number (1-based, as on MIDI gear), or the menu
    // placeholder "----", on the display.
    void show();
};

#endif // ENCODER_TASK_HPP
