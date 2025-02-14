#pragma once

#define NOINLINE __attribute__((noinline))

#include "config/config.h"

// MCU specific platform from platform/X
#include "platform_mcu.h"

#include "target/common_post.h"
