#ifndef PRESET_HPP
#define PRESET_HPP

#include <Arduino.h>
#include "AmpChannel.hpp"

// Number of presets, one per MIDI program.
#define PRESET_COUNT 128

// EEPROM address of preset 0. Each preset takes one byte, so preset n is at
// PRESET_EEPROM_BASE + n.
#define PRESET_EEPROM_BASE 0

// The amp channel stored for one MIDI program.
//
// A preset doesn't know its program number: the presets live in an array
// indexed by it (see PresetLoader), so load() and flush() take it as an
// argument to find the preset's EEPROM byte. A byte that doesn't hold a valid
// channel, e.g. 0xFF in a fresh EEPROM, loads as Clean (the factory preset).
class Preset
{
    // The channel.
    AmpChannel channel_;

public:
    // Constructor.
    Preset(AmpChannel channel = AmpChannel::clean);

    // Returns the channel.
    AmpChannel channel() const;

    // Sets the channel, in RAM only.
    void set_channel(AmpChannel channel);

    // Reads the channel from the EEPROM byte of program number.
    void load(uint8_t number);

    // Writes the channel to the EEPROM byte of program number, if it differs
    // from what is stored there.
    void flush(uint8_t number) const;
};

#endif // PRESET_HPP
