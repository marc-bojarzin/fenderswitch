#include "Button.hpp"

Button::Button(uint8_t pin)
    : pin_{pin}
{
}

void Button::begin()
{
    pinMode(pin_, INPUT_PULLUP);
    // Give the pull-up a moment to charge the line before the first read.
    delayMicroseconds(10);
    stable_ = digitalRead(pin_);
    changed_at_ = millis();
}

bool Button::pressed()
{
    uint32_t now = millis();
    if (now - changed_at_ < BUTTON_DEBOUNCE_MS)
    {
        // Still bouncing from the last change.
        return false;
    }
    uint8_t raw = digitalRead(pin_);
    if (raw == stable_)
    {
        return false;
    }
    stable_ = raw;
    changed_at_ = now;
    return raw == LOW;
}

bool Button::down() const
{
    return stable_ == LOW;
}

uint32_t Button::held_for() const
{
    return down() ? millis() - changed_at_ : 0;
}
