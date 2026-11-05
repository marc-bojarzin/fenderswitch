#include "MidiTask.hpp"

MidiTask::MidiTask(EncoderTask& ui)
    : ui_{ui}
{
}

void MidiTask::set_channel(uint8_t channel)
{
    if (channel >= 1 && channel <= 16)
    {
        channel_ = channel - 1;
    }
}

void MidiTask::set_omni(bool omni)
{
    omni_ = omni;
}

void MidiTask::run()
{
    while (Serial.available() > 0)
    {
        MidiMessage message = parser_.feed(Serial.read());
        if (message.type != MIDI_PROGRAM_CHANGE)
        {
            continue;
        }
        if (!omni_ && message.channel != channel_)
        {
            continue;
        }
        Serial.print(F("midi: program change ch="));
        Serial.print(message.channel + 1);
        Serial.print(F(" pc="));
        Serial.println(message.data1 + 1);
        ui_.select(message.data1);
    }
}
