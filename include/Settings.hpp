#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <Arduino.h>
#include "Preset.hpp"
#include "SegmentDisplay.hpp"

// Factory default for the MIDI channel, 1-16, used while OMNI is off. Only
// applies while the EEPROM holds no setting yet (fresh chip); after that the
// menu sets it.
#ifndef MIDI_CHANNEL
#define MIDI_CHANNEL 1
#endif

// Factory default for OMNI mode: 1 = listen to all channels, 0 = only
// MIDI_CHANNEL. Like MIDI_CHANNEL, only used on a fresh EEPROM.
#ifndef MIDI_OMNI
#define MIDI_OMNI 1
#endif

// Factory default for idle dimming: minutes without input before the display
// dims, 1-IDLE_DIM_MAX_MIN, 0 = never. Like MIDI_CHANNEL, only used on a fresh
// EEPROM.
#ifndef IDLE_DIM_MIN
#define IDLE_DIM_MIN 5
#endif

// Longest idle time the menu offers, in minutes.
#define IDLE_DIM_MAX_MIN 10

static_assert(MIDI_CHANNEL >= 1 && MIDI_CHANNEL <= 16, "MIDI_CHANNEL must be 1-16");
static_assert(SEGMENT_BRIGHTNESS <= 7, "SEGMENT_BRIGHTNESS must be 0-7");
static_assert(IDLE_DIM_MIN <= IDLE_DIM_MAX_MIN, "IDLE_DIM_MIN must be 0-10");

// EEPROM addresses of the settings: the bytes right after the presets.
#define SETTINGS_EEPROM_MIDI       (PRESET_EEPROM_BASE + PRESET_COUNT)
#define SETTINGS_EEPROM_BRIGHTNESS (SETTINGS_EEPROM_MIDI + 1)
#define SETTINGS_EEPROM_IDLE       (SETTINGS_EEPROM_BRIGHTNESS + 1)

// Value of midi() that means OMNI mode.
#define MIDI_OMNI_SETTING 0

// The device settings made in the menu, one EEPROM byte each:
//
//   MIDI         0 = OMNI, 1-16 = only that channel
//   brightness   of the 7-segment display, 0 (dimmest) to 7 (brightest)
//   idle         minutes without input before the display dims, 1-10,
//                0 = never
//
// A byte out of range, e.g. 0xFF in a fresh EEPROM, loads as the factory
// default (MIDI_OMNI and MIDI_CHANNEL, SEGMENT_BRIGHTNESS, IDLE_DIM_MIN).
class Settings
{
    // 0 = OMNI, 1-16 = MIDI channel.
    uint8_t midi_ = MIDI_OMNI ? MIDI_OMNI_SETTING : MIDI_CHANNEL;

    // Display brightness, 0-7.
    uint8_t brightness_ = SEGMENT_BRIGHTNESS;

    // Idle time before dimming, in minutes, 0 = never.
    uint8_t idle_ = IDLE_DIM_MIN;

public:
    // Loads the settings from EEPROM.
    void begin();

    // Returns 0 (MIDI_OMNI_SETTING) for OMNI, else the MIDI channel 1-16.
    uint8_t midi() const;

    // Returns true if a message on channel (0-15, as on the wire) is for us.
    bool accepts(uint8_t channel) const;

    // Sets the MIDI setting (0 = OMNI, 1-16 = channel) and writes it to
    // EEPROM, unless the EEPROM already holds it. Other values are ignored.
    // Returns true if the EEPROM was written.
    bool store_midi(uint8_t midi);

    // Returns the display brightness, 0-7.
    uint8_t brightness() const;

    // Sets the display brightness (0-7) and writes it to EEPROM, unless the
    // EEPROM already holds it. Other values are ignored. Doesn't touch the
    // display itself. Returns true if the EEPROM was written.
    bool store_brightness(uint8_t brightness);

    // Returns the idle time before the display dims, in minutes, 0 = never.
    uint8_t idle() const;

    // Sets the idle time (0-IDLE_DIM_MAX_MIN minutes, 0 = never) and writes
    // it to EEPROM, unless the EEPROM already holds it. Other values are
    // ignored. Returns true if the EEPROM was written.
    bool store_idle(uint8_t minutes);
};

#endif // SETTINGS_HPP
