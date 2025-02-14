#pragma once

#define NOINLINE __attribute__((noinline))

// MCU specific platform from drivers/X
#include "platform_mcu.h"

#include "config/config.h"
#include "common/utils.h"
