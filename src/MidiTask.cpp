#include "MidiTask.hpp"

MidiTask::MidiTask(EncoderTask& ui, const Settings& settings)
    : ui_{ui}
    , settings_{settings}
{
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
        if (!settings_.accepts(message.channel))
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
