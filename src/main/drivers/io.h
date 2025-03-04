#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/io_types.h"

// preprocessor is used to convert pinid to requested C data value
// compile-time error is generated if requested pin is not available (not set in TARGET_IO_PORTx)
// ioTag_t and IO_t is supported, but ioTag_t is preferred

// expand pinid to ioTag_t
#define IO_TAG(pinid) DEFIO_TAG(pinid)

// declare available IO pins. Available pins are specified per target
#include "io_def.h"

bool IORead(IO_t io);
void IOWrite(IO_t io, bool value);
void IOHi(IO_t io);
void IOLo(IO_t io);
void IOToggle(IO_t io);

void IOInit(IO_t io, uint8_t index);
void IORelease(IO_t io);  // unimplemented
bool IOIsFree(IO_t io);
IO_t IOGetByTag(ioTag_t tag);

void IOConfigGPIO(IO_t io, ioConfig_t cfg);
void IOConfigGPIOAF(IO_t io, ioConfig_t cfg, uint8_t af);

void IOInitGlobal(void);

typedef void (*IOTraverseFuncPtr_t)(IO_t io);

void IOTraversePins(IOTraverseFuncPtr_t func);

GPIO_TypeDef* IO_GPIO(IO_t io);
uint16_t IO_Pin(IO_t io);

typedef enum {
    PREINIT_PIN_STATE_NOCHANGE = 0,
    PREINIT_PIN_STATE_LOW,
    PREINIT_PIN_STATE_HIGH,
} ioPreinitPinState_e;

void ioPreinitByIO(const IO_t io, uint8_t iocfg, ioPreinitPinState_e init);
void ioPreinitByTag(ioTag_t tag, uint8_t iocfg, ioPreinitPinState_e init);
