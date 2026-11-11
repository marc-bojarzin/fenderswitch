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

void Led::brightness(uint8_t brightness)
{
    if (brightness != brightness_)
    {
        brightness_ = brightness;
        set(level_);
    }
}

void Led::set(uint8_t level)
{
    level_ = level;
    // Scaled linearly; a lit level stays at least 1 while the brightness is
    // above 0, so a dim swell never goes dark.
    uint8_t pwm = 0;
    if (level != 0 && brightness_ != 0)
    {
        pwm = static_cast<uint16_t>(level) * brightness_ / 255;
        pwm = pwm == 0 ? 1 : pwm;
    }
    // analogWrite() falls back to a plain digitalWrite() for 0 and 255.
    analogWrite(pin_, pwm);
}

void Led::off()
{
    start(led_off);
    set(0);
}

void Led::on()
{
    start(led_on);
    set(255);
}

void Led::blink_slow()
{
    if (status_ != led_blink_slow)
    {
        start(led_blink_slow);
        set(255);
    }
}

void Led::blink_fast()
{
    if (status_ != led_blink_fast)
    {
        start(led_blink_fast);
        set(255);
    }
}

void Led::swell()
{
    if (status_ != led_swell)
    {
        start(led_swell);
        phase_ = 0;
        set(swell_pwm(0));
    }
}

void Led::wink(unsigned duration)
{
    start(led_wink);
    duration_ = duration;
    set(0);
}

void Led::flash(unsigned duration)
{
    start(led_flash_once);
    duration_ = duration;
    set(255);
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
        set(0);
        status_ = led_off;
    }
    else if (status_ == led_wink)
    {
        TASK_SLEEP(duration_);
        set(255);
        status_ = led_on;
    }
    else if (status_ == led_swell)
    {
        for (;;)
        {
            TASK_SLEEP(LED_SWELL_STEP_MS);
            phase_ = (phase_ + LED_SWELL_STEP) & 0x1ff;
            set(swell_pwm(phase_ < 256 ? phase_ : 511 - phase_));
        }
    }
    else
    {
        for (;;)
        {
            TASK_SLEEP(status_ == led_blink_slow ? LED_SLOW_FREQ : LED_FAST_FREQ);
            set(level_ ? 0 : 255);
        }
    }
    TASK_END();
}

void Led::start(LedStatus status)
{
    status_ = status;
    restart();
}
