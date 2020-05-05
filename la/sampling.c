#include "sampling.h"
#include <stdlib.h>
#include "sump.h"

static uint32_t transferCount;
static uint32_t delayCount;
uint32_t triggerMask;
uint32_t triggerValue;
uint16_t flags;
uint16_t period;

void SetBufferSize(uint32_t value){
	transferCount = value;
    }

void SetDelayCount(uint32_t value){
	delayCount = value & 0xfffffffe;
    }

void SetTriggerMask(uint32_t value){
    triggerMask = value;
    }
void SetTriggerValue(uint32_t value){
    triggerValue = value;
    }
void SetFlags(uint32_t value){
    flags = value;
    }