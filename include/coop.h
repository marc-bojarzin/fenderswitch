#pragma once
#include <Arduino.h>

// Minimal cooperative multitasking (protothread style).
//
// A task's run() body is written as straight-line code. TASK_SLEEP / TASK_YIELD /
// TASK_WAIT_UNTIL return to the scheduler and the next run() resumes right after
// that statement. No per-task stack is needed, so a task costs only a few bytes.
//
// Rules inside run():
//  - Local variables do NOT survive a yield; keep state in task member variables.
//  - Do not use the TASK_* macros inside a switch statement of your own.
//  - Never call delay(); everything must yield instead.

class Task {
public:
  virtual void run() = 0;

  // Called by the scheduler: runs the task once it is no longer sleeping.
  void tick(uint32_t now) {
    if ((int32_t)(now - wakeAt_) >= 0) run();
  }

protected:
  // Abandon the current position: the next run() starts again at TASK_BEGIN.
  void restart() {
    lc_ = 0;
    wakeAt_ = millis();
  }

  uint16_t lc_ = 0;      // resume point (source line of the last yield)
  uint32_t wakeAt_ = 0;  // millis() timestamp before which run() is not called
};

#define TASK_BEGIN() switch (lc_) { case 0:
#define TASK_END()   } lc_ = 0; return

// Give other tasks a turn, continue on the next pass.
#define TASK_YIELD() \
  do { lc_ = __LINE__; return; case __LINE__:; } while (0)

// Sleep without blocking other tasks.
#define TASK_SLEEP(ms) \
  do { wakeAt_ = millis() + (ms); lc_ = __LINE__; return; case __LINE__:; } while (0)

// Poll a condition each pass until it becomes true.
#define TASK_WAIT_UNTIL(cond) \
  do { lc_ = __LINE__; case __LINE__: if (!(cond)) return; } while (0)

// Round-robin over a fixed list of tasks; call from loop().
template <size_t N>
inline void runTasks(Task* (&tasks)[N]) {
  uint32_t now = millis();
  for (size_t i = 0; i < N; ++i) tasks[i]->tick(now);
}
