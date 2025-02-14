#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "core/init.h"

#include "scheduler/scheduler.h"

void run(void);

int main(void)
{
    init();

    run();

    return 0;
}

void FAST_CODE run(void)
{
    while (true) {
        scheduler();
    }
}
