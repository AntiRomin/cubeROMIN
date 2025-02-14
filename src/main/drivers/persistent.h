#pragma once

// Available RTC backup registers (4-byte words) per MCU type
// H7: 32 words

typedef enum {
    PERSISTENT_OBJECT_MAGIC = 0,
    PERSISTENT_OBJECT_RESET_REASON,
    PERSISTENT_OBJECT_COUNT,
} persistentObjectId_e;

// Values for PERSISTENT_OBJECT_RESET_REASON
#define RESET_NONE                      0
#define RESET_BOOTLOADER_REQUEST_ROM    1  // Boot loader invocation was requested
#define RESET_BOOTLOADER_POST           2  // Reset after boot loader activity
#define RESET_MSC_REQUEST               3  // MSC invocation was requested
#define RESET_FORCED                    4  // Reset due to unknown reset reason
#define RESET_BOOTLOADER_REQUEST_FLASH  5

void persistentObjectInit(void);
uint32_t persistentObjectRead(persistentObjectId_e id);
void persistentObjectWrite(persistentObjectId_e id, uint32_t value);
