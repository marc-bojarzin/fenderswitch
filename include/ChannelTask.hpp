#ifndef CHANNEL_TASK_HPP
#define CHANNEL_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "AmpChannel.hpp"
#include "Button.hpp"
#include "EncoderTask.hpp"
#include "PresetLoader.hpp"

// The two channel buttons: Clean and Drive.
//
// Clean selects Clean. Drive selects Drive, coming from Clean; after that each
// press toggles between More Drive and Drive.
//
// A press sets the working buffer to that channel and switches the amp right
// away, without the browse delay. If the channel differs from the stored
// preset, the preset is dirty and the channel LED swells; going back to the
// stored channel makes it clean again. Only the Store button stores.
//
// The display (LEDs, 7-segment display, Serial) goes through EncoderTask, so
// both tasks show the working buffer the same way.
class ChannelTask : public Task
{
    Button& clean_;
    Button& drive_;
    PresetLoader& presets_;
    EncoderTask& ui_;

public:
    // Not default constructible.
    ChannelTask() = delete;

    // Constructor.
    ChannelTask(Button& clean, Button& drive, PresetLoader& presets,
                EncoderTask& ui);

    // Polls both buttons; called by runTasks().
    void run() override;

private:
    // Sets the working buffer to channel and shows it.
    void select(AmpChannel channel);
};

#endif // CHANNEL_TASK_HPP
