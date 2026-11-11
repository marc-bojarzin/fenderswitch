#include "DuoLed.hpp"

DuoLed::DuoLed(byte red_pin, byte green_pin)
    : red_{red_pin}
    , green_{green_pin}
{
    red();
}

void DuoLed::red()
{
    color(DUO_RED, 0);
}

void DuoLed::green()
{
    color(0, DUO_GREEN);
}

void DuoLed::yellow()
{
    color(DUO_YELLOW_RED, DUO_YELLOW_GREEN);
}

void DuoLed::color(uint8_t red, uint8_t green)
{
    red_.brightness(red);
    green_.brightness(green);
}

void DuoLed::off()
{
    red_.off();
    green_.off();
}

void DuoLed::on()
{
    red_.on();
    green_.on();
}

void DuoLed::blink_slow()
{
    red_.blink_slow();
    green_.blink_slow();
}

void DuoLed::blink_fast()
{
    red_.blink_fast();
    green_.blink_fast();
}

void DuoLed::swell()
{
    red_.swell();
    green_.swell();
}

void DuoLed::flash(unsigned duration)
{
    red_.flash(duration);
    green_.flash(duration);
}

void DuoLed::wink(unsigned duration)
{
    red_.wink(duration);
    green_.wink(duration);
}

void DuoLed::run()
{
    // One timestamp for both, so they wake in the same pass.
    uint32_t now = millis();
    red_.tick(now);
    green_.tick(now);
}
