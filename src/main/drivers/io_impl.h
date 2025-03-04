#pragma once

// TODO - GPIO_TypeDef include
#include "platform.h"

#include "drivers/io.h"

typedef struct ioDef_s {
    ioTag_t tag;
} ioDef_t;

typedef struct ioRec_s {
    GPIO_TypeDef *gpio;
    uint16_t pin;
    bool isUsed;
    uint8_t index;
} ioRec_t;

extern ioRec_t ioRecs[];

int IO_GPIOPortIdx(IO_t io);
int IO_GPIOPinIdx(IO_t io);

int IO_GPIO_PinSource(IO_t io);
int IO_GPIO_PortSource(IO_t io);

GPIO_TypeDef* IO_GPIO(IO_t io);
uint16_t IO_Pin(IO_t io);

#define IO_GPIOBYTAG(tag) IO_GPIO(IOGetByTag(tag))
#define IO_PINBYTAG(tag) IO_Pin(IOGetByTag(tag))
#define IO_GPIOPortIdxByTag(tag) DEFIO_TAG_GPIOID(tag)
#define IO_GPIOPinIdxByTag(tag) DEFIO_TAG_PIN(tag)

uint32_t IO_EXTI_Line(IO_t io);
ioRec_t *IO_Rec(IO_t io);
