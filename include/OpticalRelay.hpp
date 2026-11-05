#ifndef OPTICAL_RELAY_HPP
#define OPTICAL_RELAY_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "AmpChannel.hpp"

// Wait between the two steps of a channel change, in ms. The TLP222A needs up
// to 2 ms to turn on. Override it, e.g. with -DRELAY_SETTLE_MS=500 in
// build_flags, to make the sequence visible on prototype LEDs.
#ifndef RELAY_SETTLE_MS
#define RELAY_SETTLE_MS 5
#endif

// The two photorelays on the amp's footswitch line: relay 1 (channel) and
// relay 2 (gain). Each pin drives its relay's input LED directly, through a
// series resistor to GND: HIGH turns the relay ON (closed), LOW turns it OFF.
// While the microcontroller is in reset or booting, the pins are Hi-Z and both
// relays are open, so the amp sees More Drive until this class takes over.
//
//   Channel     OUT_CH  OUT_GAIN
//   Clean       ON      ON (ignored by the amp)
//   Drive       OFF     ON
//   More Drive  OFF     OFF
//
// select() switches make-before-break: the pin that keeps the amp in a quiet
// state goes first, then, after RELAY_SETTLE_MS, the other one. Entering Clean
// closes the channel relay first; leaving it sets the gain relay first. The
// wait is only taken when both pins actually change. The second step is
// carried out by run(), so the OpticalRelay must be in the list passed to
// runTasks().
//
// Calling select() again while a change is in progress starts over from the
// current pin levels towards the new target. Each step on its own still ends in
// a valid channel, so this never produces an unintended state.
class OpticalRelay : public Task
{
    // Pin driving relay 1 (channel: Clean / Drive).
    byte ch_pin_;

    // Pin driving relay 2 (gain: Drive / More Drive).
    byte gain_pin_;

    // The channel the relays are switching to, or are at.
    AmpChannel target_ = AmpChannel::clean;

    // True while the second step of a change is pending.
    bool pending_ = false;

    // Level for the second step, kept here because locals don't survive a
    // TASK_SLEEP.
    byte second_pin_;
    byte second_level_ = HIGH;

public:
    // Not default constructible.
    OpticalRelay() = delete;

    // Drives both pins HIGH before making them outputs, so they go straight
    // from Hi-Z to HIGH and the amp reaches Clean as early as possible. Runs
    // as a static constructor, before setup().
    OpticalRelay(byte ch_pin, byte gain_pin);

    // Starts switching to channel. Returns immediately.
    void select(AmpChannel channel);

    // Returns the channel last passed to select() (Clean at startup).
    AmpChannel channel() const;

    // Returns true while a change is still in progress.
    bool busy() const;

    // Carries out the second step; called by runTasks().
    void run() override;
};

#endif // OPTICAL_RELAY_HPP
