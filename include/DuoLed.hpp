#ifndef DUO_LED_HPP
#define DUO_LED_HPP

#include <Arduino.h>
#include "Task.hpp"
#include "Led.hpp"

// Brightness of each die in yellow, 0-255 (PWM). Their ratio sets the shade:
// lower the red one if yellow looks orange, the green one if it looks lime.
// Override them, e.g. with -DDUO_YELLOW_RED=160 in build_flags.
#ifndef DUO_YELLOW_RED
#define DUO_YELLOW_RED 255
#endif
#ifndef DUO_YELLOW_GREEN
#define DUO_YELLOW_GREEN 255
#endif

// Brightness of the die in plain red or green, 0-255 (PWM).
#ifndef DUO_RED
#define DUO_RED 255
#endif
#ifndef DUO_GREEN
#define DUO_GREEN 255
#endif

// Red/green duo LED (common cathode, 3 pins): two Leds, one per die; both
// together give yellow.
//
// Every mode goes to both Leds at once, so they stay in step; the color only
// sets each Led's brightness, a die that isn't part of the color at 0. So
// changing the color takes effect right away and keeps the mode going, e.g.
// blinking continues in the new color. Both pins must be PWM pins, to mix the
// color. Starts red.
//
// run() runs both Leds, so the DuoLed must be in the list passed to
// runTasks(), not its Leds.
class DuoLed : public Task
{
    Led red_;
    Led green_;

public:
    // Not default constructible.
    DuoLed() = delete;

    // Constructor.
    DuoLed(byte red_pin, byte green_pin);

    // Sets the color.
    void red();
    void green();
    void yellow();

    // The Led modes, on both dies; see Led.
    void off();
    void on();
    void blink_slow();
    void blink_fast();
    void swell();
    void flash(unsigned duration);
    void wink(unsigned duration);

    // Runs both Leds; called by runTasks().
    void run() override;

private:
    // Sets the brightness of both dies.
    void color(uint8_t red, uint8_t green);
};

#endif // DUO_LED_HPP
