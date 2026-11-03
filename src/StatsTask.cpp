#include "StatsTask.hpp"

void StatsTask::count()
{
    ++loopCount_;
}

void StatsTask::run()
{
#if STATS_INTERVAL_S > 0
    TASK_BEGIN();
    for (;;)
    {
        TASK_SLEEP(STATS_INTERVAL_S * 1000UL);
        Serial.print(F("t="));
        Serial.print(millis());
        Serial.print(F(" ms, loop passes/s="));
        Serial.println(loopCount_ / STATS_INTERVAL_S);
        loopCount_ = 0;
    }
    TASK_END();
#endif
}
