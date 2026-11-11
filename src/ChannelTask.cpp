#include "ChannelTask.hpp"

ChannelTask::ChannelTask(Button& clean, Button& drive, PresetLoader& presets,
                         EncoderTask& ui)
    : clean_{clean}
    , drive_{drive}
    , presets_{presets}
    , ui_{ui}
{
}

void ChannelTask::run()
{
    // Poll both buttons on every pass, so each one keeps debouncing.
    bool clean = clean_.pressed();
    bool drive = drive_.pressed();
    if (clean)
    {
        select(AmpChannel::clean);
    }
    if (drive)
    {
        select(presets_.working().channel() == AmpChannel::drive
                   ? AmpChannel::more_drive
                   : AmpChannel::drive);
    }
}

void ChannelTask::select(AmpChannel channel)
{
    presets_.set_channel(channel);
    ui_.apply();
}
