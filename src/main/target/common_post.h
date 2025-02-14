#pragma once

#include "build/version.h"

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#if defined(DEBUG)
#define FAST_CODE
#else
#define FAST_CODE                   __attribute__((section(".tcm_code")))
#endif

#define FAST_DATA_ZERO_INIT         __attribute__((section(".bss.fastram_bss"), aligned(4)))
#define FAST_DATA                   __attribute__((section(".fastram_data"), aligned(4)))

#elif defined(__GNUC__)

#endif