#ifndef ROTARY_ENCODER_HPP
#define ROTARY_ENCODER_HPP

#include <Arduino.h>
#include "Button.hpp"

// Mechanical quadrature rotary encoder with push button (e.g. EC11), decoded
// from both pins with interrupts.
//
// The sketch calls on_change() from an interrupt on every edge of CLK (A) and
// DTA (B), e.g. a pin-change interrupt covering both pins. on_change() reads
// both levels into a 2-bit code (DTA << 1) | CLK and feeds it into a Gray-code
// state machine: every valid transition between neighbouring codes adds +1 or
// -1 to a sub-step counter, invalid ones (both pins changed at once, i.e. a
// missed edge) add nothing. Contact bounce on one pin flips that pin back and
// forth, so its +1 and -1 cancel out.
//
// One detent is a full Gray-code cycle (4 sub-steps), resting at code 3 (both
// pins HIGH on the pull-ups). Each time the encoder arrives at the detent,
// the sub-steps since the last detent are settled: at least half a cycle in
// one direction counts as one step, anything less (turned and turned back) as
// none. Settling at the detent keeps the steps aligned with the clicks, even
// after a missed edge.
//
// Steps accumulate in a counter until moved() reads and clears them, so slow
// polling doesn't lose steps. Which way counts +1 depends on how the encoder
// is wired; swap CLK and DTA to reverse it.
//
// The push button (SW) is the "Enter" button: a Button (active low on the
// external and internal pull-ups, polled, debounced), read through enter_pressed().
class RotaryEncoder
{
    // CLK (clock) pin A.
    uint8_t clk_pin_;

    // DTA (data) pin B.
    uint8_t dta_pin_;

    // Last two codes, two bits each: (previous << 2) | current.
    volatile uint8_t state_ = 0;

    // Sub-steps (+1/-1 per valid transition) since the encoder last rested
    // at the detent.
    volatile int8_t sub_steps_ = 0;

    // Steps (detents) since the last call to moved(), clamped to -127..127.
    // One byte, so the main loop reads it in a single instruction, never
    // half-updated by the interrupt.
    volatile int8_t moved_ = 0;

    // SW, the push button ("Enter").
    Button sw_;

public:
    // Constructor. Doesn't touch the pins yet; call begin() in setup().
    RotaryEncoder(uint8_t clk_pin, uint8_t dta_pin, uint8_t sw_pin);

    // Sets CLK and DTA to INPUT_PULLUP, since a bare encoder only shorts them
    // to GND (C). SW gets the internal pull-up through Button. The external
    // 10k pull-ups on A, B and SW (or a KY-040 module's) sit in parallel.
    // Reads the current CLK and DTA levels as the starting point. Doesn't
    // set up the interrupt; the sketch must do that, after this call.
    void begin();

    // ISR handler: call on every edge of CLK and of DTA.
    void on_change();

    // Returns the steps since the last call (negative for the other
    // direction) and clears them.
    int moved();

    // Clears the pending steps and returns their previous value, with
    // interrupts blocked, so no step gets lost in between.
    int reset();

    // Returns true once per press of the "Enter" button (on the debounced
    // press, not the release). Poll it often, e.g. on every loop() pass.
    bool enter_pressed();

private:
    // Returns the current code, (DTA << 1) | CLK.
    uint8_t read_code() const;
};

#endif // ROTARY_ENCODER_HPP
