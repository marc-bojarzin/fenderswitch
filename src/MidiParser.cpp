#include "MidiParser.hpp"

MidiMessage MidiParser::feed(uint8_t byte)
{
    MidiMessage message;

    if (byte >= 0xF8)
    {
        // Real-time: one byte, may interrupt anything, changes nothing.
        return message;
    }
    if (byte >= 0xF0)
    {
        // System common or SysEx start/end: cancels running status. The data
        // bytes that follow are skipped, because status_ is 0.
        status_ = 0;
        count_ = 0;
        return message;
    }
    if (byte & 0x80)
    {
        // Channel status: starts a new message.
        status_ = byte;
        count_ = 0;
        return message;
    }

    // Data byte.
    if (status_ == 0)
    {
        return message;
    }
    data_[count_++] = byte;
    if (count_ < data_length(status_))
    {
        return message;
    }

    // Complete. Keep status_ for running status, start over with the data.
    count_ = 0;
    message.type = status_ & 0xF0;
    message.channel = status_ & 0x0F;
    message.data1 = data_[0];
    message.data2 = data_length(status_) == 2 ? data_[1] : 0;
    return message;
}

uint8_t MidiParser::data_length(uint8_t status)
{
    uint8_t type = status & 0xF0;
    return (type == 0xC0 || type == 0xD0) ? 1 : 2;
}
