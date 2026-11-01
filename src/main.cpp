#include <Arduino.h>
#include "coop.h"
#include "led.h"

// Sandbox: a dimmable LED on pin 9 (via 220-330 ohm resistor to GND) driven by
// a demo task, all without delay().
//  - DemoTask walks the LED through fade, brightness steps, blink and breathe,
//    announcing each phase on the serial port.
//  - StatsTask prints once a second how many times loop() ran, proving that
//    nothing blocks.

static uint32_t loopCount = 0;

static Led led(9);

class DemoTask : public Task {
public:
  void run() override {
    TASK_BEGIN();
    for (;;) {
      Serial.println(F("fade up"));
      led.fadeTo(255, 1500);
      TASK_WAIT_UNTIL(led.isIdle());

      Serial.println(F("fade down"));
      led.fadeTo(0, 1500);
      TASK_WAIT_UNTIL(led.isIdle());

      Serial.println(F("steps 0..252"));
      for (i_ = 0; i_ <= 4; ++i_) {
        led.set(i_ * 63);
        TASK_SLEEP(500);
      }

      Serial.println(F("blink"));
      led.blink(100, 300);
      TASK_SLEEP(2000);

      Serial.println(F("breathe"));
      led.breathe(2000);
      TASK_SLEEP(6000);

      led.fadeTo(0, 300);
      TASK_WAIT_UNTIL(led.isIdle());
      TASK_SLEEP(1000);
    }
    TASK_END();
  }

private:
  uint8_t i_;  // loop counter must be a member: locals are lost on yield
};

class StatsTask : public Task {
public:
  void run() override {
    TASK_BEGIN();
    for (;;) {
      TASK_SLEEP(1000);
      Serial.print(F("t="));
      Serial.print(millis());
      Serial.print(F(" ms, loop passes/s="));
      Serial.println(loopCount);
      loopCount = 0;
    }
    TASK_END();
  }
};

static DemoTask demo;
static StatsTask stats;
static Task* tasks[] = {&demo, &led, &stats};

void setup() {
  Serial.begin(9600);
  Serial.println(F("led sandbox started"));
  led.begin();
}

void loop() {
  ++loopCount;
  runTasks(tasks);
}
