#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform.h"

#include "drivers/persistent.h"
#include "drivers/system.h"

void SystemSetup(void);

void systemInit(void)
{
    SystemSetup();
}
