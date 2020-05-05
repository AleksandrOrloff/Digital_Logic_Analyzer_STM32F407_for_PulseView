#include "sampling.h"
#include <stdlib.h>
#include "sump.h"

static uint32_t transferCount;
static uint32_t delayCount;

void SetBufferSize(uint32_t value)
{
	transferCount = value;
}

void SetDelayCount(uint32_t value)
{
	delayCount = value & 0xfffffffe;
}