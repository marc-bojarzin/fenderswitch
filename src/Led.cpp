#include "Led.hpp"

// Maps a linear level (0-255) to a PWM value that looks linear to the eye:
// perceived brightness is far from linear in the duty cycle, so a plain ramp
// would seem to sit at full brightness most of the time. Squaring is a cheap
// approximation of the usual gamma curve. Never below LED_SWELL_MIN.
static uint8_t swell_pwm(uint8_t level)
{
    uint8_t pwm = static_cast<uint16_t>(level) * level / 255;
    return pwm < LED_SWELL_MIN ? LED_SWELL_MIN : pwm;
}

Led::Led(byte pin)
    : pin_{pin}
{
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW);
}

byte Led::pin() const
{
    return pin_;
}

void Led::off()
{
    start(led_off);
    digitalWrite(pin_, LOW);
}

void Led::on()
{
    start(led_on);
    digitalWrite(pin_, HIGH);
}

void Led::blink_slow()
{
    if (status_ != led_blink_slow)
    {
        start(led_blink_slow);
    }
}

void Led::blink_fast()
{
    if (status_ != led_blink_fast)
    {
        start(led_blink_fast);
    }
}

void Led::swell()
{
    if (status_ != led_swell)
    {
        start(led_swell);
        phase_ = 0;
        analogWrite(pin_, swell_pwm(0));
    }
}

void Led::wink(unsigned duration)
{
    start(led_wink);
    duration_ = duration;
    digitalWrite(pin_, LOW);
}

void Led::flash(unsigned duration)
{
    start(led_flash_once);
    duration_ = duration;
    digitalWrite(pin_, HIGH);
}

void Led::run()
{
    if (status_ == led_off || status_ == led_on)
    {
        return;
    }

    // if/else rather than switch: the TASK_* macros are built on a switch.
    TASK_BEGIN();
    if (status_ == led_flash_once)
    {
        TASK_SLEEP(duration_);
        digitalWrite(pin_, LOW);
        status_ = led_off;
    }
    else if (status_ == led_wink)
    {
        TASK_SLEEP(duration_);
        digitalWrite(pin_, HIGH);
        status_ = led_on;
    }
    else if (status_ == led_swell)
    {
        for (;;)
        {
            TASK_SLEEP(LED_SWELL_STEP_MS);
            phase_ = (phase_ + LED_SWELL_STEP) & 0x1ff;
            analogWrite(pin_, swell_pwm(phase_ < 256 ? phase_ : 511 - phase_));
        }
    }
    else
    {
        for (;;)
        {
            TASK_SLEEP(status_ == led_blink_slow ? LED_SLOW_FREQ : LED_FAST_FREQ);
            digitalWrite(pin_, !digitalRead(pin_));
        }
    }
    TASK_END();
}

void Led::start(LedStatus status)
{
    status_ = status;
    restart();
}
