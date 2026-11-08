#ifndef MIDI_TASK_HPP
#define MIDI_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "MidiParser.hpp"
#include "EncoderTask.hpp"
#include "MidiSettings.hpp"

// MIDI IN: reads the UART and recalls the preset of each Program Change.
//
// The UART's RX pin (D0) carries both the DIN socket's optocoupler and the USB
// chip's TX, so MIDI from a DIN cable and MIDI sent over USB by a serial-MIDI
// bridge on the computer arrive here the same way.
//
// A Program Change on the MIDI channel set in MidiSettings (or any channel
// with OMNI on) selects that program right away, dropping an unsaved change,
// and switches the amp. Program numbers 0-127 on the wire are programs 1-128 on the display.
// All other messages are ignored.
//
// Drains every byte waiting in the UART buffer on each pass, so the 64-byte
// receive buffer never overflows.
class MidiTask : public Task
{
    MidiParser parser_;
    EncoderTask& ui_;
    const MidiSettings& settings_;

public:
    // Not default constructible.
    MidiTask() = delete;

    // Constructor. Program changes go through ui, settings selects the
    // channel.
    MidiTask(EncoderTask& ui, const MidiSettings& settings);

    // Reads and handles all received bytes; called by runTasks().
    void run() override;
};

#endif // MIDI_TASK_HPP
