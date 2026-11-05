#ifndef PRESET_LOADER_HPP
#define PRESET_LOADER_HPP

#include <Arduino.h>
#include "AmpChannel.hpp"
#include "Preset.hpp"

// Holds all presets in RAM, plus the working buffer.
//
// begin() loads every preset from EEPROM. select() loads a preset into the
// working buffer, e.g. after a MIDI Program Change or when browsing with the
// encoder. The working buffer consists of two presets: loaded_, the preset as
// selected, and working_, which receives the changes made with the channel
// buttons through set_channel(). It is dirty while the two differ. store()
// writes working_ back to the preset and to EEPROM; select() drops it.
class PresetLoader
{
    // All presets, indexed by program number. Clean until begin() loads them.
    Preset presets_[PRESET_COUNT];

    // The selected program number.
    uint8_t number_ = 0;

    // The selected preset as loaded.
    Preset loaded_;

    // The selected preset, possibly modified.
    Preset working_;

public:
    // Loads all presets from EEPROM and selects program 0.
    void begin();

    // Loads preset number into the working buffer, dropping any unsaved
    // change. Numbers outside 0 to PRESET_COUNT - 1 are ignored.
    void select(uint8_t number);

    // Returns the selected program number.
    uint8_t number() const;

    // Returns the selected preset, possibly modified.
    const Preset& working() const;

    // Changes the channel of the working buffer.
    void set_channel(AmpChannel channel);

    // Returns true while the working buffer differs from the preset as loaded.
    bool dirty() const;

    // Writes the working buffer to the selected preset and flushes that to
    // EEPROM. Does nothing unless dirty().
    void store();
};

#endif // PRESET_LOADER_HPP
