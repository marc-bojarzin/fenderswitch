#include "led.h"

namespace {

// Brightness update interval while fading; 100 Hz looks smooth.
constexpr uint16_t kFrameMs = 10;

// Perceptual level -> PWM duty, gamma 2.2. Non-zero levels map to at least 1
// so dim fades never drop out before reaching 0.
const uint8_t kGamma[256] PROGMEM = {
    0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
    3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
    6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
   12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
   20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
   30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
   42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
   56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
   73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
   91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
  113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
  137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
  163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
  192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
  223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
};

}  // namespace

void Led::begin() {
  pinMode(pin_, OUTPUT);
  write(level_);
}

void Led::set(uint8_t level) {
  start(Mode::Idle);
  write(level);
}

void Led::fadeTo(uint8_t level, uint16_t ms) {
  startFade(level, ms);
  start(Mode::Fade);
}

void Led::breathe(uint16_t periodMs, uint8_t maxLevel) {
  durMs_ = periodMs / 2;
  peak_ = maxLevel;
  start(Mode::Breathe);
}

void Led::blink(uint16_t onMs, uint16_t offMs, uint8_t level) {
  onMs_ = onMs;
  offMs_ = offMs;
  peak_ = level;
  start(Mode::Blink);
}

void Led::run() {
  if (mode_ == Mode::Idle) return;

  // if/else rather than switch: the TASK_* macros are built on a switch.
  TASK_BEGIN();
  if (mode_ == Mode::Fade) {
    while (!fadeStep()) TASK_SLEEP(kFrameMs);
  } else if (mode_ == Mode::Breathe) {
    for (;;) {
      startFade(peak_, durMs_);
      while (!fadeStep()) TASK_SLEEP(kFrameMs);
      startFade(0, durMs_);
      while (!fadeStep()) TASK_SLEEP(kFrameMs);
    }
  } else if (mode_ == Mode::Blink) {
    for (;;) {
      write(peak_);
      TASK_SLEEP(onMs_);
      write(0);
      TASK_SLEEP(offMs_);
    }
  }
  mode_ = Mode::Idle;
  TASK_END();
}

void Led::write(uint8_t level) {
  level_ = level;
  analogWrite(pin_, pgm_read_byte(&kGamma[level]));
}

void Led::startFade(uint8_t to, uint16_t ms) {
  from_ = level_;
  to_ = to;
  durMs_ = ms;
  startMs_ = millis();
}

bool Led::fadeStep() {
  uint32_t elapsed = millis() - startMs_;
  if (elapsed >= durMs_) {
    write(to_);
    return true;
  }
  write(from_ + (int32_t)(to_ - from_) * (int32_t)elapsed / durMs_);
  return false;
}

void Led::start(Mode mode) {
  mode_ = mode;
  restart();
}
