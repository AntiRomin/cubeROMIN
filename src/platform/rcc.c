#include "platform.h"
#include "drivers/rcc.h"

void RCC_ClockCmd(rccPeriphTag_t periphTag, FunctionalState NewState)
{
    int tag = periphTag >> 5;
    uint32_t mask = 1 << (periphTag & 0x1f);

#if defined(USE_HAL_DRIVER)

// Note on "suffix" macro parameter:
// ENR and RSTR naming conventions for buses with multiple registers per bus differs among MCU types.
// ST decided to use AxBn{L,H}ENR convention for H7 which can be handled with simple "ENR" (or "RSTR") contatenation,
// while use AxBnENR{1,2} convention for G4 which requires extra "suffix" to be concatenated.
// Here, we use "suffix" for all MCU types and leave it as empty where not applicable.

#define NOSUFFIX // Empty

#define __HAL_RCC_CLK_ENABLE(bus, suffix, enbit)   do {      \
        __IO uint32_t tmpreg;                                \
        SET_BIT(RCC->bus ## ENR ## suffix, enbit);           \
        /* Delay after an RCC peripheral clock enabling */   \
        tmpreg = READ_BIT(RCC->bus ## ENR ## suffix, enbit); \
        UNUSED(tmpreg);                                      \
    } while(0)

#define __HAL_RCC_CLK_DISABLE(bus, suffix, enbit) (RCC->bus ## ENR ## suffix &= ~(enbit))

#define __HAL_RCC_CLK(bus, suffix, enbit, newState) \
    if (newState == ENABLE) {                       \
        __HAL_RCC_CLK_ENABLE(bus, suffix, enbit);   \
    } else {                                        \
        __HAL_RCC_CLK_DISABLE(bus, suffix, enbit);  \
    }

    switch (tag) {
        case RCC_AHB1:
            __HAL_RCC_CLK(AHB1, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB2:
            __HAL_RCC_CLK(AHB2, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB2:
            __HAL_RCC_CLK(APB2, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB3:
            __HAL_RCC_CLK(AHB3, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB4:
            __HAL_RCC_CLK(AHB4, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB1L:
            __HAL_RCC_CLK(APB1L, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB1H:
            __HAL_RCC_CLK(APB1H, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB3:
            __HAL_RCC_CLK(APB3, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB4:
            __HAL_RCC_CLK(APB4, NOSUFFIX, mask, NewState);
            break;
    }
}

void RCC_ResetCmd(rccPeriphTag_t periphTag, FunctionalState NewState)
{
    int tag = periphTag >> 5;
    uint32_t mask = 1 << (periphTag & 0x1f);

// Peripheral reset control relies on RSTR bits are identical to ENR bits where applicable

#define __HAL_RCC_FORCE_RESET(bus, suffix, enbit) (RCC->bus ## RSTR ## suffix |= (enbit))
#define __HAL_RCC_RELEASE_RESET(bus, suffix, enbit) (RCC->bus ## RSTR ## suffix &= ~(enbit))
#define __HAL_RCC_RESET(bus, suffix, enbit, NewState) \
    if (NewState == ENABLE) {                         \
        __HAL_RCC_RELEASE_RESET(bus, suffix, enbit);  \
    } else {                                          \
        __HAL_RCC_FORCE_RESET(bus, suffix, enbit);    \
    }

    switch (tag) {
        case RCC_AHB1:
            __HAL_RCC_RESET(AHB1, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB2:
            __HAL_RCC_RESET(AHB2, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB2:
            __HAL_RCC_RESET(APB2, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB3:
            __HAL_RCC_RESET(AHB3, NOSUFFIX, mask, NewState);
            break;

        case RCC_AHB4:
            __HAL_RCC_RESET(AHB4, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB1L:
            __HAL_RCC_RESET(APB1L, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB1H:
            __HAL_RCC_RESET(APB1H, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB3:
            __HAL_RCC_RESET(APB3, NOSUFFIX, mask, NewState);
            break;

        case RCC_APB4:
            __HAL_RCC_RESET(APB4, NOSUFFIX, mask, NewState);
            break;
    }
}
