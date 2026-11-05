#ifndef MIDI_PARSER_HPP
#define MIDI_PARSER_HPP

#include <Arduino.h>

// A complete MIDI channel message, as returned by MidiParser::feed().
struct MidiMessage
{
    // Message type: the status byte's upper nibble, e.g. 0xC0 for Program
    // Change. 0 means "no message".
    uint8_t type = 0;

    // MIDI channel, 0-15 (shown as 1-16 on MIDI gear).
    uint8_t channel = 0;

    // First and second data byte, 0-127. data2 is 0 for one-byte messages.
    uint8_t data1 = 0;
    uint8_t data2 = 0;
};

// Program Change: data1 is the program number, 0-127.
constexpr uint8_t MIDI_PROGRAM_CHANGE = 0xC0;

// Byte-by-byte MIDI parser for a serial MIDI stream (MIDI 1.0).
//
// Handles what real senders put on the wire:
//  - Running status: after one status byte, a sender may omit it for further
//    messages of the same type and channel.
//  - Real-time messages (0xF8-0xFF, e.g. MIDI clock) can appear anywhere, even
//    between the bytes of another message. They are skipped and don't disturb
//    the message in progress.
//  - System common messages and SysEx (0xF0-0xF7) cancel running status; their
//    data bytes, including a whole SysEx dump, are skipped.
//  - Data bytes without a valid status (e.g. right after power-up, mid-stream)
//    are skipped until the next status byte.
class MidiParser
{
    // Running status: the last channel status byte, or 0 if none.
    uint8_t status_ = 0;

    // Data bytes collected for the current message.
    uint8_t data_[2] = {0, 0};
    uint8_t count_ = 0;

public:
    // Feeds one received byte. Returns the message once it is complete,
    // otherwise a message with type 0.
    MidiMessage feed(uint8_t byte);

private:
    // Number of data bytes that follow status: 1 for Program Change and
    // Channel Pressure, 2 for the other channel messages.
    static uint8_t data_length(uint8_t status);
};

#endif // MIDI_PARSER_HPP
