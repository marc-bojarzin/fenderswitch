#include "ChannelTask.hpp"

ChannelTask::ChannelTask(Button& clean, Button& drive, Button& more_drive,
                         PresetLoader& presets, EncoderTask& ui)
    : buttons_{&clean, &drive, &more_drive}
    , presets_{presets}
    , ui_{ui}
{
}

void ChannelTask::run()
{
    // Poll every button on every pass, so each one keeps debouncing.
    for (uint8_t i = 0; i < AMP_CHANNEL_COUNT; ++i)
    {
        if (buttons_[i]->pressed())
        {
            presets_.set_channel(static_cast<AmpChannel>(i));
            ui_.apply();
            holding_ = i;
        }
    }

    if (holding_ == NO_HOLD)
    {
        return;
    }
    if (!buttons_[holding_]->down())
    {
        // Released before the hold time: nothing to store.
        holding_ = NO_HOLD;
    }
    else if (buttons_[holding_]->held_for() >= CHANNEL_HOLD_MS)
    {
        // Only the current channel's button stores, and only once per hold.
        if (channel_index(presets_.working().channel()) == holding_)
        {
            ui_.store();
        }
        holding_ = NO_HOLD;
    }
}
