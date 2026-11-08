#ifndef MIDI_SETTINGS_HPP
#define MIDI_SETTINGS_HPP

#include <Arduino.h>
#include "Preset.hpp"

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

static_assert(MIDI_CHANNEL >= 1 && MIDI_CHANNEL <= 16, "MIDI_CHANNEL must be 1-16");

// EEPROM address of the MIDI setting: the first byte after the presets.
#define MIDI_SETTINGS_EEPROM_ADDR (PRESET_EEPROM_BASE + PRESET_COUNT)

// Value of setting() that means OMNI mode.
#define MIDI_OMNI_SETTING 0

// The MIDI receive setting: OMNI, or one channel 1-16.
//
// Kept in a single EEPROM byte: 0 = OMNI, 1-16 = that channel. Any other
// value, e.g. 0xFF in a fresh EEPROM, loads as the factory default
// (MIDI_OMNI, MIDI_CHANNEL).
class MidiSettings
{
    // 0 = OMNI, 1-16 = MIDI channel.
    uint8_t setting_ = MIDI_OMNI ? MIDI_OMNI_SETTING : MIDI_CHANNEL;

public:
    // Loads the setting from EEPROM.
    void begin();

    // Returns 0 (MIDI_OMNI_SETTING) for OMNI, else the MIDI channel 1-16.
    uint8_t setting() const;

    // Returns true if a message on channel (0-15, as on the wire) is for us.
    bool accepts(uint8_t channel) const;

    // Sets the setting (0 = OMNI, 1-16 = channel) and writes it to EEPROM.
    // Other values are ignored.
    void store(uint8_t setting);
};

#endif // MIDI_SETTINGS_HPP
