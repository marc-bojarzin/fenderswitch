#ifndef MIDI_TASK_HPP
#define MIDI_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "MidiParser.hpp"
#include "EncoderTask.hpp"

// MIDI channel to listen to, 1-16, while OMNI is off. Default until the menu
// can set it.
#ifndef MIDI_CHANNEL
#define MIDI_CHANNEL 1
#endif

// 1 = OMNI on (listen to all channels), 0 = only MIDI_CHANNEL. Default until
// the menu can set it.
#ifndef MIDI_OMNI
#define MIDI_OMNI 1
#endif

// MIDI IN: reads the UART and recalls the preset of each Program Change.
//
// The UART's RX pin (D0) carries both the DIN socket's optocoupler and the USB
// chip's TX, so MIDI from a DIN cable and MIDI sent over USB by a serial-MIDI
// bridge on the computer arrive here the same way.
//
// A Program Change on the selected MIDI channel (or any channel with OMNI on)
// selects that program right away, dropping an unsaved change, and switches
// the amp. Program numbers 0-127 on the wire are programs 1-128 on the display.
// All other messages are ignored.
//
// Drains every byte waiting in the UART buffer on each pass, so the 64-byte
// receive buffer never overflows.
class MidiTask : public Task
{
    MidiParser parser_;
    EncoderTask& ui_;

    // MIDI channel, 0-15.
    uint8_t channel_ = MIDI_CHANNEL - 1;

    // True: accept every channel.
    bool omni_ = MIDI_OMNI;

public:
    // Not default constructible.
    MidiTask() = delete;

    // Constructor. Program changes go through ui.
    explicit MidiTask(EncoderTask& ui);

    // Sets the MIDI channel, 1-16. Other values are ignored.
    void set_channel(uint8_t channel);

    // Turns OMNI mode on or off.
    void set_omni(bool omni);

    // Reads and handles all received bytes; called by runTasks().
    void run() override;
};

#endif // MIDI_TASK_HPP
