#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "drivers/io.h"
#include "drivers/system.h"

#include "core/tasks.h"

#include "core/init.h"

void init(void)
{
    systemInit();

    // Initialize task data as soon as possible. Has to be done before tasksInit()
    // and any init code that may try to modify task behaviour before tasksInit().
    tasksInitData();

    // initialize IO (needed for all IO operations)
    IOInitGlobal();

    unusedPinsInit();

    tasksInit();
}
