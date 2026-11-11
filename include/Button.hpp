#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <Arduino.h>

// After a debounced change, changes are ignored this long, in ms. Must cover
// the contact's bounce time.
#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 30
#endif

// Momentary push button from a pin to GND, on the internal pull-up.
//
// Polled, not interrupt driven: call pressed() often, e.g. on every loop()
// pass. down() reports the state as of the last call to pressed().
//
// Debounced on the leading edge: a press counts on the first LOW reading
// after the button has rested, so it reacts within one loop pass instead of
// waiting out the bounce. A mechanical contact only bounces after it has
// closed, so the bounce is skipped by ignoring the pin for
// BUTTON_DEBOUNCE_MS after each change, the release included. The buttons
// sit on short wires inside the box; a long, noisy line would need a
// confirming second reading.
class Button
{
    // Input pin, active low.
    uint8_t pin_;

    // Debounced level (HIGH = released).
    uint8_t stable_ = HIGH;

    // millis() of the last debounced change.
    uint32_t changed_at_ = 0;

public:
    // Not default constructible.
    Button() = delete;

    // Constructor. Doesn't touch the pin yet; call begin() in setup().
    Button(uint8_t pin);

    // Sets the pin to INPUT_PULLUP and takes its level as the starting
    // state, so a button held at power-up doesn't count as a press.
    void begin();

    // Returns true once per press (on the press, not the release).
    bool pressed();

    // Returns true while the button is held down (debounced).
    bool down() const;
};

#endif // BUTTON_HPP
