#include <util/atomic.h>
#include "RotaryEncoder.hpp"

RotaryEncoder::RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin, uint8_t sw_pin)
    : clk_pin_{clk_pin}
    , dta_pin_{dta_pin}
    , sw_{sw_pin}
{
    // A bare encoder has no pull-ups: its contacts only short A/B to C (GND).
    pinMode(clk_pin_, INPUT_PULLUP);
    pinMode(dta_pin_, INPUT_PULLUP);
    // Start from the actual levels (both idle HIGH), so the
    // first edge after power-up counts like any other.
    clk_last_ = digitalRead(clk_pin_);
    dta_last_ = digitalRead(dta_pin_);
}

// Runs in interrupt context.
void RotaryEncoder::on_clk_change()
{
    uint8_t clk = digitalRead(clk_pin_);
    uint8_t dta = digitalRead(dta_pin_);
    // Count the edge only if CLK really changed and DTA moved since the last
    // counted edge; otherwise it is bounce on CLK.
    if (clk != clk_last_)
    {
        clk_last_ = clk;
        if (dta != dta_last_)
        {
            dta_last_ =  dta;
            transition(clk == dta ? 2 : 1);
        }
    }
}

// Runs in interrupt context.
//
// state_ holds the last two symbols, two bits each: the previous one in bits
// 3-2 and the current one in bits 1-0. Symbol 0 means "idle" (nothing pending)
// and only ever appears as the previous symbol, after the reset at the end of
// this function. Symbol 3 never occurs.
//
// movements[] says what the pair (previous, current) adds to moved_. It is a
// 4x4 table, laid out below with one row per previous symbol:
//
//   previous  current  adds  meaning
//   0 (idle)  1        -1    first edge after idle: latch -1
//   0 (idle)  2        +1    first edge after idle: latch +1
//   1         1         0    still turning the -1 way: already latched
//   2         2         0    still turning the +1 way: already latched
//   1         2        +1    direction reversed: cancels the -1 (jitter)
//   2         1        -1    direction reversed: cancels the +1 (jitter)
//
// Since a pair only adds something right after idle (latch) or when the
// direction reverses (cancel), moved_ stays within -1, 0 and +1. Whenever it
// ends up at 0, state_ goes back to idle, so the next edge latches again.
//
// Example, turning the -1 way and polling with moved() after every edge:
//   edge 1: (0,1) -> moved_ = -1   moved() returns -1, moved_ = 0
//   edge 2: (1,1) -> moved_ =  0   back to idle
//   edge 3: (0,1) -> moved_ = -1   moved() returns -1, ...
// So continuous rotation yields one step per two CLK edges. A bounce that
// flips the direction for one edge cancels itself:
//   edge 1: (0,1) -> moved_ = -1
//   edge 2: (1,2) -> moved_ =  0   back to idle, nothing reported
void RotaryEncoder::transition(uint8_t direction)
{
    static const int8_t movements[16] = {
    //  current:   0   1   2   3      previous:
                   0, -1, +1,  0,  // 0 (idle)
                   0,  0, +1,  0,  // 1
                   0, -1,  0,  0,  // 2
                   0,  0,  0,  0,  // 3 (never occurs)
    };

    state_  = ((state_ << 2) | direction) & 0x0f;

    moved_ += movements[state_];
    // Back to idle once nothing is pending, so the next symbol counts again.
    if (moved_ == 0) {
        state_ = 0;
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
