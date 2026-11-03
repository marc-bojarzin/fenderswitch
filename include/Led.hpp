#ifndef LED_HPP
#define LED_HPP

#include <Arduino.h>
#include "Task.hpp"

// Time between toggles while blinking, in ms.
#define LED_SLOW_FREQ 500
#define LED_FAST_FREQ 125

// Swelling: one full cycle (dark -> bright -> dark) takes 512 / LED_SWELL_STEP
// updates of LED_SWELL_STEP_MS each, about 1.9 s with the defaults.
#ifndef LED_SWELL_STEP_MS
#define LED_SWELL_STEP_MS 15
#endif
#ifndef LED_SWELL_STEP
#define LED_SWELL_STEP 4
#endif
// Lowest brightness while swelling (0-255), so the LED never looks off.
#ifndef LED_SWELL_MIN
#define LED_SWELL_MIN 6
#endif

enum LedStatus
{
    led_off         = 0
  , led_on          = 1
  , led_blink_slow  = 2
  , led_blink_fast  = 3
  , led_flash_once  = 4
  , led_swell       = 5
};

// On/off LED on a digital pin.
//
// swell() fades the LED up and down with analogWrite(), so it needs a PWM pin
// (on the Nano D3, D5, D6, D9, D10 or D11). All other modes work on any pin.
//
// on() and off() take effect immediately; blinking and flash() are carried out
// by run(), so the Led must be in the list passed to runTasks(). Every call
// replaces whatever the LED was doing before, except that calling blink_slow(),
// blink_fast() or swell() again while already in that mode keeps it going.
class Led : public Task
{
    // The output pin this LED is connected to.
    byte pin_;

    // Status of the LED.
    LedStatus status_ = led_off;

    // How long flash() keeps the LED on, in ms.
    unsigned duration_ = 0;
    // Position in the swell cycle, 0-511: rising up to 255, then falling.
    uint16_t phase_ = 0;

public:
    // Not default constructible.
    Led() = delete;

    // Constructor.
    Led(byte pin);

    // Returns the pin number.
    byte pin() const;

    // Sets the Led to OFF
    void off();

    // Sets the Led to ON
    void on();

    // Sets the Led to SLOW blinking.
    void blink_slow();

    // Sets the Led to FAST blinking.
    void blink_fast();

    // Fades the LED up and down continuously, starting dark. Needs a PWM pin.
    void swell();

    // Turns the LED on for duration ms, then off.
    void flash(unsigned duration);

    // Blinks or ends a flash; called by runTasks().
    void run() override;

private:
    // Switches to status and restarts run() from the top.
    void start(LedStatus status);

};

#endif // LED_HPP
