#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "platform.h"

#include "build/build_config.h"

#include "common/maths.h"
#include "common/time.h"
#include "common/utils.h"

#include "drivers/time.h"
#include "drivers/system.h"

#include "core/tasks.h"

#include "scheduler.h"

extern task_t tasks[];

static FAST_DATA_ZERO_INIT task_t *currentTask = NULL;
static FAST_DATA_ZERO_INIT bool ignoreCurrentTaskExecRate;
static FAST_DATA_ZERO_INIT bool ignoreCurrentTaskExecTime;

int32_t taskGuardCycles;
static int32_t taskGuardMinCycles;
static int32_t taskGuardMaxCycles;
static uint32_t taskGuardDeltaDownCycles;
static uint32_t taskGuardDeltaUpCycles;

FAST_DATA_ZERO_INIT uint16_t averageSystemLoadPercent = 0;

static FAST_DATA_ZERO_INIT int taskQueuePos = 0;
STATIC_UNIT_TESTED FAST_DATA_ZERO_INIT int taskQueueSize = 0;

static uint32_t lastTargetCycles;

static int16_t lateTaskCount = 0;
static uint32_t lateTaskTotal = 0;
static int16_t taskCount = 0;
static uint32_t lateTaskPercentage = 0;
static uint32_t nextTimingCycles;

STATIC_UNIT_TESTED FAST_DATA_ZERO_INIT task_t* taskQueueArray[TASK_COUNT + 1];

void queueClear(void)
{
    memset(taskQueueArray, 0, sizeof(taskQueueArray));
    taskQueuePos = 0;
    taskQueueSize = 0;
}

static bool queueContains(const task_t *task)
{
    for (int ii = 0; ii < taskQueueSize; ++ii) {
        if (taskQueueArray[ii] == task) {
            return true;
        }
    }
    return false;
}

bool queueAdd(task_t *task)
{
    if ((taskQueueSize >= TASK_COUNT) || queueContains(task)) {
        return false;
    }
    for (int ii = 0; ii <= taskQueueSize; ++ii) {
        if (taskQueueArray[ii] == NULL || taskQueueArray[ii]->attribute->staticPriority < task->attribute->staticPriority) {
            memmove(&taskQueueArray[ii+1], &taskQueueArray[ii], sizeof(task) * (taskQueueSize - ii));
            taskQueueArray[ii] = task;
            ++taskQueueSize;
            return true;
        }
    }
    return false;
}

bool queueRemove(task_t *task)
{
    for (int ii = 0; ii < taskQueueSize; ++ii) {
        if (taskQueueArray[ii] == task) {
            memmove(&taskQueueArray[ii], &taskQueueArray[ii+1], sizeof(task) * (taskQueueSize - ii));
            --taskQueueSize;
            return true;
        }
    }
    return false;
}

/*
 * Returns first item queue or NULL if queue empty
 */
FAST_CODE task_t *queueFirst(void)
{
    taskQueuePos = 0;
    return taskQueueArray[0]; // guaranteed to be NULL if queue is empty
}

/*
 * Returns next item in queue or NULL if at end of queue
 */
FAST_CODE task_t *queueNext(void)
{
    return taskQueueArray[++taskQueuePos]; // guaranteed to be NULL at end of queue
}

static timeUs_t taskTotalExecutionTime = 0;

void taskSystemLoad(timeUs_t currentTimeUs)
{
    static timeUs_t lastExecutedAtUs;
    timeDelta_t deltaTime = cmpTimeUs(currentTimeUs, lastExecutedAtUs);

    // Calculate system load
    if (deltaTime) {
        averageSystemLoadPercent = 100 * taskTotalExecutionTime / deltaTime;
        taskTotalExecutionTime = 0;
        lastExecutedAtUs = currentTimeUs;
    } else {
        schedulerIgnoreTaskExecTime();
    }
}

uint32_t getCpuPercentageLate(void)
{
    return lateTaskPercentage;
}

timeUs_t checkFuncMaxExecutionTimeUs;
timeUs_t checkFuncTotalExecutionTimeUs;
timeUs_t checkFuncMovingSumExecutionTimeUs;
timeUs_t checkFuncMovingSumDeltaTimeUs;

void getCheckFuncInfo(cfCheckFuncInfo_t *checkFuncInfo)
{
    checkFuncInfo->maxExecutionTimeUs = checkFuncMaxExecutionTimeUs;
    checkFuncInfo->totalExecutionTimeUs = checkFuncTotalExecutionTimeUs;
    checkFuncInfo->averageExecutionTimeUs = checkFuncMovingSumExecutionTimeUs / TASK_STATS_MOVING_SUM_COUNT;
    checkFuncInfo->averageDeltaTimeUs = checkFuncMovingSumDeltaTimeUs / TASK_STATS_MOVING_SUM_COUNT;
}

void getTaskInfo(taskId_e taskId, taskInfo_t *taskInfo)
{
    taskInfo->isEnabled = queueContains(getTask(taskId));
    taskInfo->desiredPeriodUs = getTask(taskId)->attribute->desiredPeriodUs;
    taskInfo->staticPriority = getTask(taskId)->attribute->staticPriority;
    taskInfo->taskName = getTask(taskId)->attribute->taskName;
    taskInfo->subTaskName = getTask(taskId)->attribute->subTaskName;
    taskInfo->maxExecutionTimeUs = getTask(taskId)->maxExecutionTimeUs;
    taskInfo->totalExecutionTimeUs = getTask(taskId)->totalExecutionTimeUs;
    taskInfo->averageExecutionTime10thUs = getTask(taskId)->movingSumExecutionTime10thUs / TASK_STATS_MOVING_SUM_COUNT;
    taskInfo->averageDeltaTime10thUs = getTask(taskId)->movingSumDeltaTime10thUs / TASK_STATS_MOVING_SUM_COUNT;
    taskInfo->latestDeltaTimeUs = getTask(taskId)->taskLatestDeltaTimeUs;
    taskInfo->movingAverageCycleTimeUs = getTask(taskId)->movingAverageCycleTimeUs;
    taskInfo->lateCount = getTask(taskId)->lateCount;
    taskInfo->runCount = getTask(taskId)->runCount;
    taskInfo->execTime = getTask(taskId)->execTime;
}

void rescheduleTask(taskId_e taskId, timeDelta_t newPeriodUs)
{
    task_t *task;

    if (taskId == TASK_SELF) {
        task = currentTask;
    } else if (taskId < TASK_COUNT) {
        task = getTask(taskId);
    } else {
        return;
    }
    task->attribute->desiredPeriodUs = MAX(SCHEDULER_DELAY_LIMIT, newPeriodUs);  // Limit delay to 100us (10 kHz) to prevent scheduler clogging
}

void setTaskEnabled(taskId_e taskId, bool enabled)
{
    if (taskId == TASK_SELF || taskId < TASK_COUNT) {
        task_t *task = taskId == TASK_SELF ? currentTask : getTask(taskId);
        if (enabled && task->attribute->taskFunc) {
            queueAdd(task);
        } else {
            queueRemove(task);
        }
    }
}

timeDelta_t getTaskDeltaTimeUs(taskId_e taskId)
{
    if (taskId == TASK_SELF) {
        return currentTask->taskLatestDeltaTimeUs;
    } else if (taskId < TASK_COUNT) {
        return getTask(taskId)->taskLatestDeltaTimeUs;
    } else {
        return 0;
    }
}

// Called by tasks executing what are known to be short states
void schedulerIgnoreTaskStateTime(void)
{
    ignoreCurrentTaskExecRate = true;
    ignoreCurrentTaskExecTime = true;
}

// Called by tasks with state machines to only count one state as determining rate
void schedulerIgnoreTaskExecRate(void)
{
    ignoreCurrentTaskExecRate = true;
}

// Called by tasks without state machines executing in what is known to be a shorter time than peak
void schedulerIgnoreTaskExecTime(void)
{
    ignoreCurrentTaskExecTime = true;
}

bool schedulerGetIgnoreTaskExecTime(void)
{
    return ignoreCurrentTaskExecTime;
}

void schedulerResetTaskStatistics(taskId_e taskId)
{
    if (taskId == TASK_SELF) {
        currentTask->anticipatedExecutionTime = 0;
        currentTask->movingSumDeltaTime10thUs = 0;
        currentTask->totalExecutionTimeUs = 0;
        currentTask->maxExecutionTimeUs = 0;
    } else if (taskId < TASK_COUNT) {
        getTask(taskId)->anticipatedExecutionTime = 0;
        getTask(taskId)->movingSumDeltaTime10thUs = 0;
        getTask(taskId)->totalExecutionTimeUs = 0;
        getTask(taskId)->maxExecutionTimeUs = 0;
    }
}

void schedulerResetTaskMaxExecutionTime(taskId_e taskId)
{
    if (taskId == TASK_SELF) {
        currentTask->maxExecutionTimeUs = 0;
    } else if (taskId < TASK_COUNT) {
        task_t *task = getTask(taskId);
        task->maxExecutionTimeUs = 0;
        task->lateCount = 0;
        task->runCount = 0;
    }
}

void schedulerResetCheckFunctionMaxExecutionTime(void)
{
    checkFuncMaxExecutionTimeUs = 0;
}

void schedulerInit(void)
{
    queueClear();
    queueAdd(getTask(TASK_SYSTEM));

    taskGuardMinCycles = clockMicrosToCycles(TASK_GUARD_MARGIN_MIN_US);
    taskGuardMaxCycles = clockMicrosToCycles(TASK_GUARD_MARGIN_MAX_US);
    taskGuardCycles = taskGuardMinCycles;
    taskGuardDeltaDownCycles = clockMicrosToCycles(1) / TASK_GUARD_MARGIN_DOWN_STEP;
    taskGuardDeltaUpCycles = clockMicrosToCycles(1) / TASK_GUARD_MARGIN_UP_STEP;

    lastTargetCycles = getCycleCounter();

    nextTimingCycles = lastTargetCycles;

    for (taskId_e taskId = 0; taskId < TASK_COUNT; taskId++) {
        schedulerResetTaskStatistics(taskId);
    }
}

static timeDelta_t taskNextStateTime;

FAST_CODE void schedulerSetNextStateTime(timeDelta_t nextStateTime)
{
    taskNextStateTime = nextStateTime;
}

FAST_CODE timeDelta_t schedulerGetNextStateTime(void)
{
    return currentTask->anticipatedExecutionTime >> TASK_EXEC_TIME_SHIFT;
}

FAST_CODE timeUs_t schedulerExecuteTask(task_t *selectedTask, timeUs_t currentTimeUs)
{
    timeUs_t taskExecutionTimeUs = 0;

    if (selectedTask) {
        currentTask = selectedTask;
        ignoreCurrentTaskExecRate = false;
        ignoreCurrentTaskExecTime = false;
        taskNextStateTime = -1;
        float period = currentTimeUs - selectedTask->lastExecutedAtUs;
        selectedTask->lastExecutedAtUs = currentTimeUs;
        selectedTask->lastDesiredAt += selectedTask->attribute->desiredPeriodUs;
        selectedTask->dynamicPriority = 0;

        // Execute task
        const timeUs_t currentTimeBeforeTaskCallUs = micros();
        selectedTask->attribute->taskFunc(currentTimeBeforeTaskCallUs);
        taskExecutionTimeUs = micros() - currentTimeBeforeTaskCallUs;
        taskTotalExecutionTime += taskExecutionTimeUs;
        selectedTask->movingSumExecutionTime10thUs += (taskExecutionTimeUs * 10) - selectedTask->movingSumExecutionTime10thUs / TASK_STATS_MOVING_SUM_COUNT;
        if (!ignoreCurrentTaskExecRate) {
            // Record task execution rate and max execution time
            selectedTask->taskLatestDeltaTimeUs = cmpTimeUs(currentTimeUs, selectedTask->lastStatsAtUs);
            selectedTask->movingSumDeltaTime10thUs += (selectedTask->taskLatestDeltaTimeUs * 10) - selectedTask->movingSumDeltaTime10thUs / TASK_STATS_MOVING_SUM_COUNT;
            selectedTask->lastStatsAtUs = currentTimeUs;
        }

        // Update estimate of expected task duration
        if (taskNextStateTime != -1) {
            selectedTask->anticipatedExecutionTime = taskNextStateTime << TASK_EXEC_TIME_SHIFT;
        } else if (!ignoreCurrentTaskExecTime) {
            if (taskExecutionTimeUs > (selectedTask->anticipatedExecutionTime >> TASK_EXEC_TIME_SHIFT)) {
                selectedTask->anticipatedExecutionTime = taskExecutionTimeUs << TASK_EXEC_TIME_SHIFT;
            } else if (selectedTask->anticipatedExecutionTime > 1) {
                // Slowly decay the max time
                selectedTask->anticipatedExecutionTime--;
            }
        }

        if (!ignoreCurrentTaskExecTime) {
            selectedTask->maxExecutionTimeUs = MAX(selectedTask->maxExecutionTimeUs, taskExecutionTimeUs);
        }

        selectedTask->totalExecutionTimeUs += taskExecutionTimeUs;   // time consumed by scheduler + task
        selectedTask->movingAverageCycleTimeUs += 0.05f * (period - selectedTask->movingAverageCycleTimeUs);
        selectedTask->runCount++;
    }

    return taskExecutionTimeUs;
}

FAST_CODE void scheduler(void)
{
    static uint32_t scheduleCount = 0;
#if !defined(DEBUG)
    const timeUs_t schedulerStartTimeUs = micros();
#endif
    timeUs_t currentTimeUs;
    uint32_t nowCycles;
    timeUs_t taskExecutionTimeUs = 0;
    task_t *selectedTask = NULL;
    uint16_t selectedTaskDynamicPriority = 0;

    nowCycles = getCycleCounter();
    currentTimeUs = micros();

    // Update task dynamic priorities
    for (task_t *task = queueFirst(); task != NULL; task = queueNext()) {
        // Task has checkFunc - event driven
        if (task->attribute->checkFunc) {
            // Increase priority for event driven tasks
            if (task->dynamicPriority > 0) {
                task->taskAgePeriods = 1 + (cmpTimeUs(currentTimeUs, task->lastSignaledAtUs) / task->attribute->desiredPeriodUs);
                task->dynamicPriority = 1 + task->attribute->staticPriority * task->taskAgePeriods;
            } else if (task->attribute->checkFunc(currentTimeUs, cmpTimeUs(currentTimeUs, task->lastExecutedAtUs))) {
                const uint32_t checkFuncExecutionTimeUs = cmpTimeUs(micros(), currentTimeUs);
                checkFuncMovingSumExecutionTimeUs += checkFuncExecutionTimeUs - checkFuncMovingSumExecutionTimeUs / TASK_STATS_MOVING_SUM_COUNT;
                checkFuncMovingSumDeltaTimeUs += task->taskLatestDeltaTimeUs - checkFuncMovingSumDeltaTimeUs / TASK_STATS_MOVING_SUM_COUNT;
                checkFuncTotalExecutionTimeUs += checkFuncExecutionTimeUs;   // time consumed by scheduler + task
                checkFuncMaxExecutionTimeUs = MAX(checkFuncMaxExecutionTimeUs, checkFuncExecutionTimeUs);
                task->lastSignaledAtUs = currentTimeUs;
                task->taskAgePeriods = 1;
                task->dynamicPriority = 1 + task->attribute->staticPriority;
            } else {
                task->taskAgePeriods = 0;
            }
        } else {
            // Task is time-driven, dynamicPriority is last execution age (measured in desiredPeriods)
            // Task age is calculated from last execution
            task->taskAgePeriods = (cmpTimeUs(currentTimeUs, task->lastExecutedAtUs) / task->attribute->desiredPeriodUs);
            if (task->taskAgePeriods > 0) {
                task->dynamicPriority = 1 + task->attribute->staticPriority * task->taskAgePeriods;
            }
        }

        if (task->dynamicPriority > selectedTaskDynamicPriority) {
            // Don't block the SERIAL task.
            if (((scheduleCount & SCHED_TASK_DEFER_MASK) == 0) ||
                ((task - tasks) == TASK_SERIAL)) {
                selectedTaskDynamicPriority = task->dynamicPriority;
                selectedTask = task;
            }
        }
    }

    if (selectedTask) {
        timeDelta_t taskRequiredTimeUs = selectedTask->anticipatedExecutionTime >> TASK_EXEC_TIME_SHIFT;
        selectedTask->execTime = taskRequiredTimeUs;
        int32_t taskRequiredTimeCycles = (int32_t)clockMicrosToCycles((uint32_t)taskRequiredTimeUs);

        nowCycles = getCycleCounter();

        // Allow a little extra time
        taskRequiredTimeCycles += taskGuardCycles;

        uint32_t antipatedEndCycles = nowCycles + taskRequiredTimeCycles;
        taskExecutionTimeUs += schedulerExecuteTask(selectedTask, currentTimeUs);
        nowCycles = getCycleCounter();
        int32_t cyclesOverdue = cmpTimeCycles(nowCycles, antipatedEndCycles);

        if (cyclesOverdue > 0) {
            if ((currentTask - tasks) != TASK_SERIAL) {
                currentTask->lateCount++;
                lateTaskCount++;
                lateTaskTotal += cyclesOverdue;
            }
        }

        if ((cyclesOverdue > 0) || (-cyclesOverdue < taskGuardMinCycles)) {
            if (taskGuardCycles < taskGuardMaxCycles) {
                taskGuardCycles += taskGuardDeltaUpCycles;
            }
        } else if (taskGuardCycles > taskGuardMinCycles) {
            taskGuardCycles -= taskGuardDeltaDownCycles;
        }
        taskCount++;
    }

#if defined(DEBUG)
    UNUSED(taskExecutionTimeUs);
#else
    UNUSED(taskExecutionTimeUs);
    UNUSED(schedulerStartTimeUs);
#endif

    scheduleCount++;
}

uint16_t getAverageSystemLoadPercent(void)
{
    return averageSystemLoadPercent;
}
