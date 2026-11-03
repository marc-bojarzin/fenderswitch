#include "OpticalRelay.hpp"

OpticalRelay::OpticalRelay(byte ch_pin, byte gain_pin)
    : ch_pin_{ch_pin}
    , gain_pin_{gain_pin}
    , second_pin_{ch_pin}
{
    // HIGH first: the pins switch from Hi-Z straight to driving HIGH.
    digitalWrite(ch_pin_, HIGH);
    digitalWrite(gain_pin_, HIGH);
    pinMode(ch_pin_, OUTPUT);
    pinMode(gain_pin_, OUTPUT);
}

void OpticalRelay::select(AmpChannel channel)
{
    byte ch_level = channel == AmpChannel::clean ? HIGH : LOW;
    byte gain_level = channel == AmpChannel::more_drive ? LOW : HIGH;

    // Entering Clean: channel relay first. Otherwise: gain relay first.
    byte first_pin = channel == AmpChannel::clean ? ch_pin_ : gain_pin_;
    byte first_level = channel == AmpChannel::clean ? ch_level : gain_level;
    second_pin_ = channel == AmpChannel::clean ? gain_pin_ : ch_pin_;
    second_level_ = channel == AmpChannel::clean ? gain_level : ch_level;

    target_ = channel;
    restart();

    bool first_changes = digitalRead(first_pin) != first_level;
    digitalWrite(first_pin, first_level);

    // Only wait if the first step changed something and a second one follows.
    pending_ = digitalRead(second_pin_) != second_level_;
    if (pending_ && !first_changes)
    {
        digitalWrite(second_pin_, second_level_);
        pending_ = false;
    }
}

AmpChannel OpticalRelay::channel() const
{
    return target_;
}

bool OpticalRelay::busy() const
{
    return pending_;
}

void OpticalRelay::run()
{
    if (!pending_)
    {
        return;
    }

    TASK_BEGIN();
    TASK_SLEEP(RELAY_SETTLE_MS);
    digitalWrite(second_pin_, second_level_);
    pending_ = false;
    TASK_END();
}
