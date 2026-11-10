#ifndef RELAY_HPP
#define RELAY_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "AmpChannel.hpp"

// Wait between the two steps of a channel change, in ms. It covers the first
// relay's operate or release time plus contact bounce (signal relays: up to
// about 10 ms). Override it, e.g. with -DRELAY_SETTLE_MS=500 in build_flags,
// to make the sequence visible on prototype LEDs.
#ifndef RELAY_SETTLE_MS
#define RELAY_SETTLE_MS 20
#endif

// The two relays on the amp's footswitch line: relay 1 (channel) and relay 2
// (gain), wired with their normally closed (NC) contacts. Each pin drives its
// relay's coil through an NPN transistor: HIGH energizes the coil and opens
// the contact, LOW releases it and the contact closes. While the
// microcontroller is in reset or booting, the pins are Hi-Z, the base
// pull-downs keep both coils off and both contacts closed, so the amp sees
// Clean.
//
//   Channel     CH contact  GAIN contact      OUT_CH  OUT_GAIN
//   Clean       closed      closed (ignored)  LOW     LOW
//   Drive       open        closed            HIGH    LOW
//   More Drive  open        open              HIGH    HIGH
//
// select() switches make-before-break: the pin that keeps the amp in a quiet
// state goes first, then, after RELAY_SETTLE_MS, the other one. Entering Clean
// closes the channel contact first; leaving it sets the gain contact first.
// The wait is only taken when both pins actually change. The second step is
// carried out by run(), so the Relay must be in the list passed to runTasks().
//
// Calling select() again while a change is in progress starts over from the
// current pin levels towards the new target. Each step on its own still ends in
// a valid channel, so this never produces an unintended state.
class Relay : public Task
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
    byte second_level_ = LOW;

public:
    // Not default constructible.
    Relay() = delete;

    // Drives both pins LOW (Clean, coils off) and makes them outputs. Runs as
    // a static constructor, before setup().
    Relay(byte ch_pin, byte gain_pin);

    // Starts switching to channel. Returns immediately.
    void select(AmpChannel channel);

    // Returns the channel last passed to select() (Clean at startup).
    AmpChannel channel() const;

    // Returns true while a change is still in progress.
    bool busy() const;

    // Carries out the second step; called by runTasks().
    void run() override;
};

#endif // RELAY_HPP
