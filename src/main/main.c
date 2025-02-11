#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "core/init.h"

void run(void);

int main(void)
{
    init();

    run();
}

void run(void)
{
    while (true) {
        __NOP();
    }
}
