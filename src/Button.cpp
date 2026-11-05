#include "Button.hpp"

Button::Button(uint8_t pin)
    : pin_{pin}
{
    pinMode(pin_, INPUT_PULLUP);
}

bool Button::pressed()
{
    uint8_t raw = digitalRead(pin_);
    uint32_t now = millis();
    if (raw != raw_)
    {
        // Still bouncing: restart the debounce interval.
        raw_ = raw;
        changed_at_ = now;
        return false;
    }
    if (raw == stable_ || now - changed_at_ < BUTTON_DEBOUNCE_MS)
    {
        return false;
    }
    stable_ = raw;
    if (raw == LOW)
    {
        pressed_at_ = now;
    }
    return raw == LOW;
}

bool Button::down() const
{
    return stable_ == LOW;
}

uint32_t Button::held_for() const
{
    return down() ? millis() - pressed_at_ : 0;
}
