#ifndef CHANNEL_TASK_HPP
#define CHANNEL_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "AmpChannel.hpp"
#include "Button.hpp"
#include "EncoderTask.hpp"
#include "PresetLoader.hpp"

// How long a channel button has to be held to store the preset, in ms.
#ifndef CHANNEL_HOLD_MS
#define CHANNEL_HOLD_MS 2000
#endif

// The three channel buttons: Clean, Drive and More Drive.
//
// Pressing one sets the working buffer to that channel and switches the amp
// right away, without the browse delay. If the channel differs from the stored
// preset, the preset is dirty and the channel LED swells; pressing the stored
// channel's button makes it clean again.
//
// Holding the button of the current channel for CHANNEL_HOLD_MS while the
// preset is dirty stores it, like the Store button. Each hold stores at most
// once; the button has to be released before the next hold counts.
//
// The display (LEDs, 7-segment display, Serial) and storing go through EncoderTask, so both
// tasks show the working buffer the same way.
class ChannelTask : public Task
{
    // Buttons indexed by AmpChannel: Clean, Drive, More Drive.
    Button* buttons_[AMP_CHANNEL_COUNT];
    PresetLoader& presets_;
    EncoderTask& ui_;

    // Index of the button whose hold is being timed, or NO_HOLD.
    uint8_t holding_ = NO_HOLD;
    static const uint8_t NO_HOLD = 0xff;

public:
    // Not default constructible.
    ChannelTask() = delete;

    // Constructor.
    ChannelTask(Button& clean, Button& drive, Button& more_drive,
                PresetLoader& presets, EncoderTask& ui);

    // Polls the three buttons; called by runTasks().
    void run() override;
};

#endif // CHANNEL_TASK_HPP
