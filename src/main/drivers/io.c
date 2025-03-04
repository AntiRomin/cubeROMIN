#include "platform.h"

#include "drivers/io.h"
#include "drivers/io_impl.h"
#include "drivers/rcc.h"

#include "common/utils.h"

// io ports defs are stored in array by index now
struct ioPortDef_s {
    rccPeriphTag_t rcc;
};

ioRec_t* IO_Rec(IO_t io)
{
    return io;
}

GPIO_TypeDef* IO_GPIO(IO_t io)
{
    const ioRec_t *ioRec = IO_Rec(io);
    return ioRec->gpio;
}

uint16_t IO_Pin(IO_t io)
{
    const ioRec_t *ioRec = IO_Rec(io);
    return ioRec->pin;
}

int IO_GPIO_PortSource(IO_t io)
{
    return IO_GPIOPortIdx(io);
}

// zero based pin index
int IO_GPIOPinIdx(IO_t io)
{
    if (!io) {
        return -1;
    }
    return 31 - __builtin_clz(IO_Pin(io));
}

int IO_GPIO_PinSource(IO_t io)
{
    return IO_GPIOPinIdx(io);
}

// claim IO pin, set owner and resources
void IOInit(IO_t io, uint8_t index)
{
    if (!io) {
        return;
    }
    ioRec_t *ioRec = IO_Rec(io);
    ioRec->isUsed = true;
    ioRec->index = index;
}

void IORelease(IO_t io)
{
    if (!io) {
        return;
    }
    ioRec_t *ioRec = IO_Rec(io);
    ioRec->isUsed = false;
}

bool IOIsFree(IO_t io)
{
    if (!io) {
        return false;
    }
    const ioRec_t *ioRec = IO_Rec(io);
    return !ioRec->isUsed;
}

#if DEFIO_IO_USED_COUNT
ioRec_t ioRecs[DEFIO_IO_USED_COUNT];
#else
// Avoid -Wpedantic warning
ioRec_t ioRecs[1];
#endif

void IOTraversePins(IOTraverseFuncPtr_t fnPtr)
{
    for (int i = 0; i < DEFIO_IO_USED_COUNT; i++) {
        fnPtr(&ioRecs[i]);
    }
}
