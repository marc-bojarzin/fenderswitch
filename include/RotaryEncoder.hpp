#ifndef ROTARY_ENCODER_HPP
#define ROTARY_ENCODER_HPP

#include <Arduino.h>
#include "Button.hpp"

// Mechanical quadrature rotary encoder with push button (e.g. EC11) read via a pin-change interrupt on
// CLK.
//
// The sketch attaches an interrupt for CHANGE on the CLK pin and calls
// on_clk_change() from it. A CLK edge only counts if DTA has also changed since
// the last counted edge, which drops CLK contact bounce. On each counted edge,
// the direction is taken from whether CLK and DTA are equal:
//   CLK != DTA -> symbol 1 -> -1
//   CLK == DTA -> symbol 2 -> +1
// Which way is clockwise depends on how the encoder is wired.
//
// transition() remembers the last two symbols and looks the pair up in a
// table. As a result, moved_ only ever holds -1, 0 or +1. It works like a
// latch, not a counter:
//   - The first edge after an idle state latches the direction.
//   - Further edges in the same direction leave it at +-1.
//   - An edge in the opposite direction cancels it back to 0 (jitter).
// moved() reads and clears the latch. The next edge after that only re-arms
// the state machine, and the edge after that latches again. So with frequent
// polling, continuous rotation yields one step per two CLK edges, and polling
// less often loses steps.
//
// The push button (SW) is the "Enter" button: a Button (active low on the
// internal pull-up, polled, debounced), read through enter_pressed().
class RotaryEncoder
{
    // CLK (clock) pin A.
    uint8_t clk_pin_;

    // DTA (data) pin B.
    uint8_t dta_pin_;

    // CLK level at the last counted edge, read from the pin at construction.
    volatile uint8_t clk_last_ = 0;

    // DTA level at the last counted edge, read from the pin at construction.
    volatile uint8_t dta_last_ = 0;

    // Last two direction symbols (1 or 2), two bits each: (previous << 2) | current.
    // 0 means idle, i.e. no pending movement.
    volatile uint8_t state_ = 0;

    // Pending movement since the last call to moved(): -1, 0 or +1. One byte,
    // so the main loop reads it in a single instruction, never half-updated
    // by the interrupt.
    volatile int8_t moved_ = 0;

    // SW, the push button ("Enter").
    Button sw_;

public:
    // Sets CLK and DTA to INPUT_PULLUP, since a bare encoder only shorts them
    // to GND (C). SW gets the internal pull-up through Button. External
    // pull-ups (e.g. on a KY-040 module) work as well.
    // Reads the current CLK and DTA levels as the starting point. Doesn't
    // attach the interrupt; the sketch must do that.
    RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin, uint8_t sw_pin);

    // Feeds one direction symbol (1 or 2, see above) into the state machine:
    // latches -1 or +1 in moved_ on the first edge after idle, and cancels it
    // back to 0 when the direction reverses. See RotaryEncoder.cpp for the
    // lookup table and examples. Called from on_clk_change(); normally not
    // called directly.
    void transition(uint8_t direction);

    // ISR handler: call on every CLK edge (attachInterrupt(..., CHANGE)).
    void on_clk_change();

    // Returns the pending movement (-1, 0 or +1) and clears it.
    int moved();

    // Clears the pending movement and returns its previous value, with
    // interrupts blocked, so no edge gets lost in between. Leaves state_
    // untouched.
    int reset();

    // Returns true once per press of the "Enter" button (on the debounced
    // press, not the release). Poll it often, e.g. on every loop() pass.
    bool enter_pressed();
};

#endif // ROTARY_ENCODER_HPP
