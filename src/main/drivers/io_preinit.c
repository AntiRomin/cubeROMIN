#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/io.h"
#include "drivers/system.h"

static unsigned preinitIndex = 0;

void ioPreinitByIO(const IO_t io, uint8_t iocfg, ioPreinitPinState_e init)
{
    if (!io) {
        return;
    }
    preinitIndex++;

    IOInit(io, preinitIndex);
    IOConfigGPIO(io, iocfg);

    switch (init) {
        case PREINIT_PIN_STATE_LOW:
            IOLo(io);
            break;
        case PREINIT_PIN_STATE_HIGH:
            IOHi(io);
            break;
        default:
            break;
    }
}

void ioPreinitByTag(ioTag_t tag, uint8_t iocfg, ioPreinitPinState_e init)
{
    ioPreinitByIO(IOGetByTag(tag), iocfg, init);
}
