#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "drivers/system.h"

#include "core/init.h"

void init(void)
{
    systemInit();
}
