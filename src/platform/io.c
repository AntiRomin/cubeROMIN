#include "platform.h"

#include "drivers/io.h"
#include "drivers/io_impl.h"
#include "drivers/rcc.h"

#include "common/utils.h"

// io ports defs are stored in array by index now
struct ioPortDef_s {
    rccPeriphTag_t rcc;
};

const struct ioPortDef_s ioPortDefs[] = {
    { RCC_AHB4(GPIOA) },
    { RCC_AHB4(GPIOB) },
    { RCC_AHB4(GPIOC) },
    { RCC_AHB4(GPIOD) },
    { RCC_AHB4(GPIOE) },
    { RCC_AHB4(GPIOF) },
    { RCC_AHB4(GPIOG) },
    { RCC_AHB4(GPIOH) },
    { RCC_AHB4(GPIOI) },
};

uint32_t IO_EXTI_Line(IO_t io)
{
    if (!io) {
        return 0;
    }
    return 1 << IO_GPIOPinIdx(io);
}

bool IORead(IO_t io)
{
    if (!io) {
        return false;
    }
    return !! HAL_GPIO_ReadPin(IO_GPIO(io), IO_Pin(io));
}

void IOWrite(IO_t io, bool hi)
{
    if (!io) {
        return;
    }
    HAL_GPIO_WritePin(IO_GPIO(io), IO_Pin(io), hi ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void IOHi(IO_t io)
{
    if (!io) {
        return;
    }
    HAL_GPIO_WritePin(IO_GPIO(io), IO_Pin(io), GPIO_PIN_SET);
}

void IOLo(IO_t io)
{
    if (!io) {
        return;
    }
    HAL_GPIO_WritePin(IO_GPIO(io), IO_Pin(io), GPIO_PIN_RESET);
}

void IOToggle(IO_t io)
{
    if (!io) {
        return;
    }
    HAL_GPIO_TogglePin(IO_GPIO(io), IO_Pin(io));
}

void IOConfigGPIO(IO_t io, ioConfig_t cfg)
{
    IOConfigGPIOAF(io, cfg, 0);
}

void IOConfigGPIOAF(IO_t io, ioConfig_t cfg, uint8_t af)
{
    if (!io) {
        return;
    }

    rccPeriphTag_t rcc = ioPortDefs[IO_GPIOPortIdx(io)].rcc;
    RCC_ClockCmd(rcc, ENABLE);

    GPIO_InitTypeDef init = {
        .Pin = IO_Pin(io),
        .Mode = (cfg >> 0) & 0x13,
        .Speed = (cfg >> 2) & 0x03,
        .Pull = (cfg >> 5) & 0x03,
        .Alternate = af
    };

    HAL_GPIO_Init(IO_GPIO(io), &init);
}
