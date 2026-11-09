#include <util/atomic.h>
#include "RotaryEncoder.hpp"

// Code where the encoder rests in a detent: both pins HIGH on the pull-ups.
static constexpr uint8_t DETENT_CODE = 0b11;

RotaryEncoder::RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin, uint8_t sw_pin)
    : clk_pin_{clk_pin}
    , dta_pin_{dta_pin}
    , sw_{sw_pin}
{
}

void RotaryEncoder::begin()
{
    sw_.begin();
    // A bare encoder has no pull-ups: its contacts only short A/B to C (GND).
    pinMode(clk_pin_, INPUT_PULLUP);
    pinMode(dta_pin_, INPUT_PULLUP);
    // Start from the actual levels, so the first edge after power-up is
    // decoded like any other.
    state_ = read_code();
}

uint8_t RotaryEncoder::read_code() const
{
    return (digitalRead(dta_pin_) << 1) | digitalRead(clk_pin_);
}

// Runs in interrupt context.
//
// Reading both pins and combining them into a 2-bit direction code with possible values
// 00, 01, 10, 11 (gray code). Each transition represents a possible movement of the encoder.
// Ignore invalid transitions if both pins change simultaneously 00 -> 11, 01 -> 10, etc (bouncing).
// Only valid transitions with correct gray code sequence are considered.
// CW : 00 -> 01 -> 11 -> 10 -> 00  (decrement)
// CCW: 00 -> 10 -> 11 -> 01 -> 00  (increment)
void RotaryEncoder::on_change()
{
    // 16 possible 4-bit states (previous 2 bits + current 2 bits)
    static const int8_t movements[16] = {

        0 , -1 , +1 ,  0    // 00->00, 00->01, 00->10, 00->11
     ,  1 ,  0 ,  0 , -1    // 01->00, 01->01, 01->10, 01->11,
     , -1 ,  0 ,  0 ,  1    // 10->00, 10->01, 10->10, 10->11
     ,  0,   1 , -1 ,  0    // 11->00, 11->01, 11->10, 11->11
    };

    uint8_t code = read_code();

    // Calculate the new state by shifting in the current direction and masking to 4 bits.
    state_ = ((state_ << 2) | code) & 0x0f;

    // Update the movement counter based on the current transition.
    sub_steps_ += movements[state_];

    if (code == DETENT_CODE)
    {
        // Settle at the detent: a full cycle gives +-4, a missed edge +-3 or
        // +-2, turning back or bounce 0.
        if (sub_steps_ >= 2 && moved_ < INT8_MAX)
        {
            ++moved_;
        }
        else if (sub_steps_ <= -2 && moved_ > -INT8_MAX)
        {
            --moved_;
        }
        sub_steps_ = 0;
    }
}

int RotaryEncoder::moved()
{
    return (moved_ ? reset() : 0);
}

int RotaryEncoder::reset()
{
    int8_t wrk;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        wrk = moved_;
        moved_ = 0;
    }
    return wrk;
}

bool RotaryEncoder::enter_pressed()
{
    return sw_.pressed();
}
