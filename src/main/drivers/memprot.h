#pragma once

typedef struct mpuRegion_s {
    uint32_t start;
    uint32_t end;        // Zero if determined by size member (MPU_REGION_SIZE_xxx)
    uint8_t  size;       // Zero if determined from linker symbols
    uint8_t  perm;
    uint8_t  exec;
    uint8_t  shareable;
    uint8_t  cacheable;
    uint8_t  bufferable;
} mpuRegion_t;

extern mpuRegion_t mpuRegions[];
extern unsigned mpuRegionCount;

void memProtReset(void);
void memProtConfigure(mpuRegion_t *mpuRegions, unsigned regionCount);
