#include <EEPROM.h>
#include "MidiSettings.hpp"

void MidiSettings::begin()
{
    uint8_t stored = EEPROM.read(MIDI_SETTINGS_EEPROM_ADDR);
    if (stored <= 16)
    {
        setting_ = stored;
    }
}

uint8_t MidiSettings::setting() const
{
    return setting_;
}

bool MidiSettings::accepts(uint8_t channel) const
{
    return setting_ == MIDI_OMNI_SETTING || channel + 1 == setting_;
}

void MidiSettings::store(uint8_t setting)
{
    if (setting <= 16)
    {
        setting_ = setting;
        EEPROM.update(MIDI_SETTINGS_EEPROM_ADDR, setting_);
    }
}
