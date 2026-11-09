#include <EEPROM.h>
#include "Settings.hpp"

void Settings::begin()
{
    uint8_t stored = EEPROM.read(SETTINGS_EEPROM_MIDI);
    if (stored <= 16)
    {
        midi_ = stored;
    }
    stored = EEPROM.read(SETTINGS_EEPROM_BRIGHTNESS);
    if (stored <= 7)
    {
        brightness_ = stored;
    }
}

uint8_t Settings::midi() const
{
    return midi_;
}

bool Settings::accepts(uint8_t channel) const
{
    return midi_ == MIDI_OMNI_SETTING || channel + 1 == midi_;
}

// Writes value to the EEPROM byte at address unless it already holds it;
// returns true if it wrote.
static bool write_if_changed(int address, uint8_t value)
{
    if (EEPROM.read(address) == value)
    {
        return false;
    }
    EEPROM.write(address, value);
    return true;
}

bool Settings::store_midi(uint8_t midi)
{
    if (midi > 16)
    {
        return false;
    }
    midi_ = midi;
    return write_if_changed(SETTINGS_EEPROM_MIDI, midi_);
}

uint8_t Settings::brightness() const
{
    return brightness_;
}

bool Settings::store_brightness(uint8_t brightness)
{
    if (brightness > 7)
    {
        return false;
    }
    brightness_ = brightness;
    return write_if_changed(SETTINGS_EEPROM_BRIGHTNESS, brightness_);
}
