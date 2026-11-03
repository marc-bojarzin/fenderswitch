#include "PresetLoader.hpp"

void PresetLoader::begin()
{
    for (uint8_t i = 0; i < PRESET_COUNT; ++i)
    {
        presets_[i].load(i);
    }
    select(0);
}

void PresetLoader::select(uint8_t number)
{
    if (number < PRESET_COUNT)
    {
        number_ = number;
        loaded_ = presets_[number];
        working_ = loaded_;
    }
}

uint8_t PresetLoader::number() const
{
    return number_;
}

const Preset& PresetLoader::working() const
{
    return working_;
}

void PresetLoader::set_channel(AmpChannel channel)
{
    working_.set_channel(channel);
}

bool PresetLoader::dirty() const
{
    return working_.channel() != loaded_.channel();
}

void PresetLoader::store()
{
    if (dirty())
    {
        presets_[number_] = working_;
        loaded_ = working_;
        presets_[number_].flush(number_);
    }
}
