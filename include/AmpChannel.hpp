#ifndef AMP_CHANNEL_HPP
#define AMP_CHANNEL_HPP

#include <stdint.h>

// The amp's three channels, as selected by the original footswitch. The values
// are stable: they index lookup tables and are stored in the EEPROM presets.
// One byte, so it fits a preset without conversion.
enum class AmpChannel : uint8_t
{
    clean       = 0
  , drive       = 1
  , more_drive  = 2
};

// Number of channels.
constexpr uint8_t AMP_CHANNEL_COUNT = 3;

// Returns the channel's value, e.g. to index a lookup table.
constexpr uint8_t channel_index(AmpChannel channel)
{
    return static_cast<uint8_t>(channel);
}

#endif // AMP_CHANNEL_HPP
