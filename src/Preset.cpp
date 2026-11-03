#include <EEPROM.h>
#include "Preset.hpp"

Preset::Preset(AmpChannel channel)
    : channel_{channel}
{
}

AmpChannel Preset::channel() const
{
    return channel_;
}

void Preset::set_channel(AmpChannel channel)
{
    channel_ = channel;
}

void Preset::load(uint8_t number)
{
    uint8_t stored = EEPROM.read(PRESET_EEPROM_BASE + number);
    channel_ = stored < AMP_CHANNEL_COUNT ? static_cast<AmpChannel>(stored)
                                          : AmpChannel::clean;
}

void Preset::flush(uint8_t number) const
{
    EEPROM.update(PRESET_EEPROM_BASE + number, channel_index(channel_));
}
