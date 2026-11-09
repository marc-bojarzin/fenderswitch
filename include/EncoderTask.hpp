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
#include "Settings.hpp"

// How long the encoder has to rest before the amp switches to the browsed
// program, in ms. Browsing past programs only updates the display, so the amp
// doesn't jump through every channel on the way.
#ifndef BROWSE_SETTLE_MS
#define BROWSE_SETTLE_MS 150
#endif

// How long the channel LED goes dark when there is nothing to store, in ms.
#ifndef NOTHING_TO_STORE_MS
#define NOTHING_TO_STORE_MS 150
#endif

// How long the display shows "donE" after a preset was stored, in ms.
#ifndef STORE_DONE_MS
#define STORE_DONE_MS 600
#endif

// How long the colon lights up when a MIDI Program Change arrives, in ms.
#ifndef MIDI_COLON_MS
#define MIDI_COLON_MS 60
#endif

// After this long without input (encoder, buttons, MIDI), the display dims to
// its lowest brightness until the next input, in ms. 0 = never dim.
#ifndef IDLE_DIM_MS
#define IDLE_DIM_MS 300000UL
#endif

// Browses the presets with the rotary encoder.
//
// Every step selects the next (+1) or previous (-1) program, wrapping around
// between the first and the last. Selecting drops an unsaved change in the
// working buffer and shows the program number on the display right away. Once
// the encoder has rested for BROWSE_SETTLE_MS, the amp switches to the preset's
// channel and its LED lights up.
//
// Pressing "Enter" opens the settings menu. Turning browses the entries of
// the current menu, wrapping around; "Enter" or "Store" picks the one shown:
//
//   Chan   MIDI channel submenu, opens on the current setting:
//          ALL (OMNI), CH:01 ... CH:16, ESC
//   dISP   display brightness submenu, opens on the current level:
//          br:01 (dimmest) ... br:08 (brightest), ESC. The display takes
//          each level right away as a preview; ESC goes back to the stored one.
//   ESC    leave the menu
//
// Picking a value in a submenu writes it to EEPROM and leaves the menu; ESC in
// a submenu goes back to the top menu without saving. Once the menu is left,
// the display shows the program number again. MIDI and the channel buttons
// keep working while the menu is open.
//
// The "Store" button next to the encoder stores a dirty working buffer in live
// mode, as an alternative to holding the channel button for 2 s.
//
// It also owns the display of the working buffer, used by ChannelTask too: the
// LED of the active channel is steady while the preset is clean and swells
// while it is dirty.
//
// On top of the program number, the display shows for a moment:
//   - "donE" for STORE_DONE_MS after a preset was stored,
//   - the colon for MIDI_COLON_MS when a MIDI Program Change arrives,
// and after IDLE_DIM_MS without input it dims to its lowest brightness; the
// next input brings the stored brightness back.
//
// A state machine:
//
//   live      the amp plays the selected program. Enter -> menu, Store ->
//             store a dirty preset, turn -> browsing.
//   browsing  the display follows the encoder, the amp waits. Enter or Store
//             apply right away, then act as in live; turn -> stay (restarts
//             the wait); rest for BROWSE_SETTLE_MS -> apply, live.
//   menu, menu_midi, menu_display
//             the top menu and its submenus. Turn -> next/previous entry;
//             Enter or Store -> pick it (open a submenu, save and -> live,
//             or ESC: up one level).
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
        menu,           // top menu
        menu_midi,      // MIDI submenu
        menu_display,   // display brightness submenu
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
    Settings& settings_;

    // Channel LEDs: Clean, Drive and More Drive.
    Led& led_green_;
    Led& led_yellow_;
    Led& led_red_;

    State state_ = State::live;

    // Entry shown in the current menu, 0 to menuSize() - 1; the last one is
    // always ESC.
    uint8_t menu_entry_ = 0;

    // millis() of the last encoder step, while browsing.
    uint32_t moved_at_ = 0;

    // millis() of the last input, and whether the display is dimmed since.
    uint32_t active_at_ = 0;
    bool dimmed_ = false;

    // "donE" after storing, and the colon after a MIDI Program Change: shown
    // while set, cleared STORE_DONE_MS / MIDI_COLON_MS after their start.
    bool done_ = false;
    uint32_t done_at_ = 0;
    bool midi_colon_ = false;
    uint32_t midi_colon_at_ = 0;

public:
    // Not default constructible.
    EncoderTask() = delete;

    // Constructor.
    EncoderTask(RotaryEncoder& encoder, Button& store, PresetLoader& presets,
                OpticalRelay& relay, SegmentDisplay& display,
                Settings& settings, Led& led_green, Led& led_yellow, Led& led_red);

    // Polls the encoder, its button and the Store button; called by
    // runTasks().
    void run() override;

    // Selects program number (0-127) right away after a MIDI Program Change:
    // drops an unsaved change, switches the amp without the browse delay,
    // cancels a pending browse and flashes the colon.
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

    // Opens the top menu. -> menu.
    void openMenu();

    // Switches to menu state (menu, menu_midi or menu_display) on entry and
    // shows it.
    void enterMenu(State state, uint8_t entry);

    // Picks the entry shown in the current menu: opens a submenu, saves a
    // value and leaves (-> live), or ESC goes up one level.
    void confirmMenu();

    // Leaves the menu. -> live.
    void closeMenu();

    // Returns the number of entries in the current menu, ESC included.
    uint8_t menuSize() const;

    // Returns true if the entry shown is the ESC of the current menu.
    bool atEsc() const;

    // Writes the text of the menu entry shown to text (4 characters at
    // most), e.g. "CH05"; returns true if the colon belongs between the
    // second and third character ("CH:05").
    bool menuText(char text[SEGMENT_DIGITS + 1]) const;

    // Writes the menu entry to Serial, e.g. "menu: CH:05".
    void printMenuEntry();

    // Counts as input: restarts the idle time, undims the display and drops
    // "donE".
    void wake();

    // Dims the display once it has been idle for IDLE_DIM_MS, and ends the
    // "donE" and colon overlays when their time is up.
    void runTimers();

    // Sets the display brightness (0-7) and reports it on Serial, e.g.
    // "display: brightness=5" (1-8, as in the menu).
    void setBrightness(uint8_t brightness);

    // Shows the program number (1-based, as on MIDI gear) with the working
    // buffer's channel as a bar symbol in the leftmost digit, "donE", or the
    // menu entry, on the display.
    void show();
};

#endif // ENCODER_TASK_HPP
