#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "platform.h"

#include "common/utils.h"

#include "scheduler/scheduler.h"

#include "tasks.h"

static void taskMain(timeUs_t currentTimeUs)
{
    UNUSED(currentTimeUs);
}

static void taskHandleSerial(timeUs_t currentTimeUs)
{
    UNUSED(currentTimeUs);
}

#ifdef DEBUG
static void taskTest(timeUs_t currentTimeUs)
{
    UNUSED(currentTimeUs);
}
#endif

#define DEFINE_TASK(taskNameParam, subTaskNameParam, checkFuncParam, taskFuncParam, desiredPeriodParam, staticPriorityParam) {  \
    .taskName = taskNameParam, \
    .subTaskName = subTaskNameParam, \
    .checkFunc = checkFuncParam, \
    .taskFunc = taskFuncParam, \
    .desiredPeriodUs = desiredPeriodParam, \
    .staticPriority = staticPriorityParam \
}

// Task info in .bss (unitialised data)
task_t tasks[TASK_COUNT];

// Task ID data in .data (initialised data)
task_attribute_t task_attributes[TASK_COUNT] = {

    [TASK_SYSTEM] = DEFINE_TASK("SYSTEM", "LOAD", NULL, taskSystemLoad, TASK_PERIOD_HZ(10), TASK_PRIORITY_MEDIUM_HIGH),
    [TASK_MAIN] = DEFINE_TASK("SYSTEM", "UPDATE", NULL, taskMain, TASK_PERIOD_HZ(1000), TASK_PRIORITY_MEDIUM_HIGH),
#ifdef DEBUG
    [TASK_DEBUG] = DEFINE_TASK("SYSTEM", "DEBUG", NULL, taskTest, TASK_PERIOD_HZ(1000), TASK_PRIORITY_LOW),
#endif
    [TASK_SERIAL] = DEFINE_TASK("SERIAL", NULL, NULL, taskHandleSerial, TASK_PERIOD_HZ(100), TASK_PRIORITY_LOW), // 100 Hz should be enough to flush up to 115 bytes @ 115200 baud
};

task_t *getTask(unsigned taskId)
{
    return &tasks[taskId];
}

// Has to be done before tasksInit() in order to initialize any task data which may be uninitialized at boot
void tasksInitData(void)
{
    for (int i = 0; i < TASK_COUNT; i++) {
        tasks[i].attribute = &task_attributes[i];
    }
}

void tasksInit(void)
{
    schedulerInit();

    setTaskEnabled(TASK_MAIN, true);

#ifdef DEBUG
    setTaskEnabled(TASK_DEBUG, true);
#endif

    setTaskEnabled(TASK_SERIAL, true);
    // rescheduleTask(TASK_SERIAL, TASK_PERIOD_HZ(serialConfig()->serial_update_rate_hz));
}
