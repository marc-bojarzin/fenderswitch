#pragma once
#include <Arduino.h>
#include "coop.h"

// Dimmable LED on a PWM pin (Nano: 3, 5, 6, 9, 10, 11).
//
// Brightness is perceptual 0..255 and gamma-corrected, so 128 looks like half.
// set() takes effect immediately; fadeTo(), breathe() and blink() are carried
// out by run(), so the Led must be in the list passed to runTasks().
// Every call replaces whatever the LED was doing before.

class Led : public Task {
public:
  explicit Led(uint8_t pin) : pin_(pin) {}

  void begin();

  void set(uint8_t level);
  void fadeTo(uint8_t level, uint16_t ms);
  void breathe(uint16_t periodMs, uint8_t maxLevel = 255);
  void blink(uint16_t onMs, uint16_t offMs, uint8_t level = 255);

  uint8_t level() const { return level_; }
  bool isIdle() const { return mode_ == Mode::Idle; }

  void run() override;

private:
  enum class Mode : uint8_t { Idle, Fade, Breathe, Blink };

  void write(uint8_t level);
  void startFade(uint8_t to, uint16_t ms);
  bool fadeStep();  // true once the fade has reached its target
  void start(Mode mode);

  uint8_t pin_;
  uint8_t level_ = 0;
  Mode mode_ = Mode::Idle;

  // fade in progress
  uint8_t from_ = 0;
  uint8_t to_ = 0;
  uint16_t durMs_ = 0;
  uint32_t startMs_ = 0;

  // breathe / blink parameters
  uint8_t peak_ = 255;
  uint16_t onMs_ = 0;
  uint16_t offMs_ = 0;
};
