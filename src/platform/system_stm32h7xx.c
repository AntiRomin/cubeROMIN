#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "platform.h"

#include "drivers/nvic.h"
#include "drivers/persistent.h"
#include "drivers/system.h"

void SystemSetup(void);

void systemInit(void)
{
    SystemSetup();

    // Configure NVIC preempt/priority groups
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITY_GROUPING);

    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();

    // Init cycle counter
    cycleCounterInit();

    // SysTick is updated whenever HAL_RCC_ClockConfig is called.
}

void systemReset(void)
{
    SCB_DisableDCache();
    SCB_DisableICache();

    __disable_irq();
    NVIC_SystemReset();
}

void systemResetWithoutDisablingCaches(void)
{
    __disable_irq();
    NVIC_SystemReset();
}

void systemResetToBootloader(bootloaderRequestType_e requestType)
{
    switch (requestType) {
#if defined(USE_FLASH_BOOT_LOADER)
    case BOOTLOADER_REQUEST_FLASH:
        persistentObjectWrite(PERSISTENT_OBJECT_RESET_REASON, RESET_BOOTLOADER_REQUEST_FLASH);

        break;
#endif
    case BOOTLOADER_REQUEST_ROM:
    default:
        persistentObjectWrite(PERSISTENT_OBJECT_RESET_REASON, RESET_BOOTLOADER_REQUEST_ROM);

        break;
    }

    __disable_irq();
    NVIC_SystemReset();
}

#define SYSMEMBOOT_VECTOR_TABLE ((uint32_t *)0x1ff09800)

typedef void *(*bootJumpPtr)(void);

static void systemJumpToBootloader(void)
{
    __SYSCFG_CLK_ENABLE();

    uint32_t bootStack = SYSMEMBOOT_VECTOR_TABLE[0];

    bootJumpPtr SysMemBootJump = (bootJumpPtr)SYSMEMBOOT_VECTOR_TABLE[1];

    __set_MSP(bootStack); //Set the main stack pointer to its default values

    SysMemBootJump();

    while (1);
}

void systemProcessResetReason(void)
{
    uint32_t bootloaderRequest = persistentObjectRead(PERSISTENT_OBJECT_RESET_REASON);

    switch (bootloaderRequest) {
#if defined(USE_FLASH_BOOT_LOADER)
    case RESET_BOOTLOADER_REQUEST_FLASH:
#endif
    case RESET_BOOTLOADER_REQUEST_ROM:
        persistentObjectWrite(PERSISTENT_OBJECT_RESET_REASON, RESET_BOOTLOADER_POST);
        systemJumpToBootloader();

        break;

    case RESET_FORCED:
        persistentObjectWrite(PERSISTENT_OBJECT_RESET_REASON, RESET_NONE);
        break;

    case RESET_BOOTLOADER_POST:
        // Boot loader activity magically prevents SysTick from interrupting.
        // Issue a soft reset to prevent the condition.
        persistentObjectWrite(PERSISTENT_OBJECT_RESET_REASON, RESET_FORCED);
        systemResetWithoutDisablingCaches(); // observed that disabling dcache after cold boot with BOOT pin high causes segfault.

        break;

    case RESET_MSC_REQUEST:
    case RESET_NONE:
    default:
        break;

    }
}
