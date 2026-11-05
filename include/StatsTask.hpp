#ifndef STATS_TASK_HPP
#define STATS_TASK_HPP

#include <Arduino.h>
#include "Task.hpp"

// Time between two reports, in seconds; 0 turns the report off (default).
// Turn it on in build_flags, e.g. -DSTATS_INTERVAL_S=10.
#ifndef STATS_INTERVAL_S
#define STATS_INTERVAL_S 0
#endif

// Development monitor: prints every STATS_INTERVAL_S over Serial how many
// times per second loop() ran (averaged over the interval), proving that
// nothing blocks.
//
// Call count() once per loop() pass. Serial output still works once MIDI IN
// uses the UART (RX only), but at the MIDI baud rate of 31250.
class StatsTask : public Task
{
    // loop() passes since the last report.
    uint32_t loopCount_ = 0;

public:
    // Counts one loop() pass.
    void count();

    // Prints and resets the counter every STATS_INTERVAL_S; called by
    // runTasks(). Does nothing if STATS_INTERVAL_S is 0.
    void run() override;
};

#endif // STATS_TASK_HPP
