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
#include "MidiSettings.hpp"

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
// Pressing "Enter" opens the MIDI menu. Turning then browses the entries
// "ALL" (OMNI), "CH:01" to "CH:16" and "ESC", wrapping around; the menu opens
// on the current setting. Pressing "Enter" or "Store" confirms the entry
// shown: a channel or ALL is written to EEPROM, ESC leaves without saving.
// Either way the display goes back to the program number. MIDI and the
// channel buttons keep working while the menu is open.
//
// The "Store" button next to the encoder stores a dirty working buffer in live
// mode, as an alternative to holding the channel button for 2 s.
//
// It also owns the display of the working buffer, used by ChannelTask too: the
// LED of the active channel is steady while the preset is clean and swells
// while it is dirty.
//
// A state machine with three states:
//
//   live      the amp plays the selected program. Enter -> menu, Store ->
//             store a dirty preset, turn -> browsing.
//   browsing  the display follows the encoder, the amp waits. Enter or Store
//             apply right away, then act as in live; turn -> stay (restarts
//             the wait); rest for BROWSE_SETTLE_MS -> apply, live.
//   menu      the MIDI menu. Enter or Store -> confirm, live; turn ->
//             next/previous entry.
//
// A button press wins over encoder steps in the same pass.
//
// A MIDI Program Change (select()) applies right away in every state; in
// browsing that ends the wait (-> live), the menu stays open.
//
// Polls every pass: reads the encoder and both buttons once, then hands them
// to the handler of the current state. moved() accumulates the steps between
// polls, so a slow pass doesn't lose any; several steps are applied at once.
class EncoderTask : public Task
{
    // See the state machine above.
    enum class State : uint8_t
    {
        live,
        browsing,
        menu,
    };

    // Input of one pass, read once and handed to the state's handler.
    struct Input
    {
        int step;     // encoder steps since the last pass, + = right
        bool enter;   // encoder button pressed
        bool store;   // Store button pressed
    };

    RotaryEncoder& encoder_;
    Button& store_;
    PresetLoader& presets_;
    OpticalRelay& relay_;
    SegmentDisplay& display_;
    MidiSettings& midi_;

    // Channel LEDs: Clean, Drive and More Drive.
    Led& led_green_;
    Led& led_yellow_;
    Led& led_red_;

    State state_ = State::live;

    // Menu entry shown: MIDI_OMNI_SETTING (ALL), 1-16 (channel) or
    // MENU_ESC.
    uint8_t menu_entry_ = MIDI_OMNI_SETTING;

    // millis() of the last encoder step, while browsing.
    uint32_t moved_at_ = 0;

public:
    // Not default constructible.
    EncoderTask() = delete;

    // Constructor.
    EncoderTask(RotaryEncoder& encoder, Button& store, PresetLoader& presets,
                OpticalRelay& relay, SegmentDisplay& display,
                MidiSettings& midi, Led& led_green, Led& led_yellow, Led& led_red);

    // Polls the encoder, its button and the Store button; called by
    // runTasks().
    void run() override;

    // Selects program number (0-127) right away, e.g. after a MIDI Program
    // Change: drops an unsaved change, switches the amp without the browse
    // delay and cancels a pending browse.
    void select(uint8_t number);

    // Switches the amp to the working buffer's channel and shows it on the
    // LEDs, Serial and the display. Ends browsing (-> live).
    void apply();

    // Stores a dirty working buffer in its preset and EEPROM and shows it as
    // clean again. On a clean preset it only acknowledges the request: the
    // channel LED goes dark for NOTHING_TO_STORE_MS.
    void store();

private:
    // State handlers, one per State; each one does the transitions.
    void runLive(const Input& in);
    void runBrowsing(const Input& in);
    void runMenu(const Input& in);

    // Selects the program step programs away (wrapping around) and shows it;
    // the amp follows once the encoder rests. -> browsing.
    void browse(int step);

    // Lights the LED of the working buffer's channel only: steady while clean,
    // swelling while dirty.
    void showChannel();

    // Returns the LED of channel.
    Led& channelLed(AmpChannel channel);

    // Opens the MIDI menu on the current setting. -> menu.
    void openMenu();

    // Confirms the menu entry shown (stores it, or drops it on ESC) and
    // leaves the menu. -> live.
    void confirmMenu();

    // Writes the menu entry to Serial, e.g. "menu: CH:05".
    void printMenuEntry();

    // Shows the program number (1-based, as on MIDI gear), or the menu entry,
    // on the display.
    void show();
};

#endif // ENCODER_TASK_HPP
