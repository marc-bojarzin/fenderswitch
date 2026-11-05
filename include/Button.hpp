#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <Arduino.h>

// Momentary push button from a pin to GND, on the internal pull-up.
//
// Polled, not interrupt driven: call pressed() often, e.g. on every loop()
// pass. A level change only counts once it has been stable for
// BUTTON_DEBOUNCE_MS. down() and held_for() report the state as of the last
// call to pressed().
class Button
{
    // Input pin, active low.
    uint8_t pin_;

    // Debounced level (HIGH = released).
    uint8_t stable_ = HIGH;

    // Level at the last call to pressed().
    uint8_t raw_ = HIGH;

    // millis() of the last change in raw_.
    uint32_t changed_at_ = 0;
    // millis() of the last debounced press.
    uint32_t pressed_at_ = 0;

    // The level must be stable this long before a change is accepted, in ms.
    static const uint8_t BUTTON_DEBOUNCE_MS = 30;

public:
    // Not default constructible.
    Button() = delete;

    // Sets pin to INPUT_PULLUP.
    Button(uint8_t pin);

    // Returns true once per press (on the debounced press, not the release).
    bool pressed();

    // Returns true while the button is held down (debounced).
    bool down() const;

    // Returns how long the button has been held down, in ms, or 0 while it is
    // released.
    uint32_t held_for() const;
};

#endif // BUTTON_HPP
