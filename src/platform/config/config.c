#include "platform.h"

#include "drivers/memprot.h"

// MPU part
mpuRegion_t mpuRegions[] = {
    {
        // Mark ITCM-RAM as read-only
        // "For Cortex®-M7, TCMs memories always behave as Non-cacheable, Non-shared normal memories, irrespective of the memory type attributes defined in the MPU for a memory region containing addresses held in the TCM"
        // See AN4838
        .start      = 0x00000000,
        .end        = 0, // Size defined by "size"
        .size       = MPU_REGION_SIZE_64KB,
        .perm       = MPU_REGION_PRIV_RO_URO,
        .exec       = MPU_INSTRUCTION_ACCESS_ENABLE,
        .shareable  = MPU_ACCESS_NOT_SHAREABLE,
        .cacheable  = MPU_ACCESS_NOT_CACHEABLE,
        .bufferable = MPU_ACCESS_BUFFERABLE,
    },
};

unsigned mpuRegionCount = ARRAYLEN(mpuRegions);
