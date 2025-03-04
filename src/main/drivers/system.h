#pragma once

#include <stdint.h>
#include <stdbool.h>

void systemInit(void);

typedef enum {
    BOOTLOADER_REQUEST_ROM,
    BOOTLOADER_REQUEST_FLASH,
} bootloaderRequestType_e;

// bootloader/IAP
void systemReset(void);
void systemResetToBootloader(bootloaderRequestType_e requestType);
void cycleCounterInit(void);
int32_t clockCyclesToMicros(int32_t clockCycles);
float clockCyclesToMicrosf(int32_t clockCycles);
int32_t clockCyclesTo10thMicros(int32_t clockCycles);
int32_t clockCyclesTo100thMicros(int32_t clockCycles);
uint32_t clockMicrosToCycles(uint32_t micros);
uint32_t getCycleCounter(void);
void systemProcessResetReason(void);

// memory
void initialiseMemorySections(void);
void systemResetWithoutDisablingCaches(void);

void unusedPinsInit(void);
