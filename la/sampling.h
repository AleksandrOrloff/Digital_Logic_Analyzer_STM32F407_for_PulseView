#include <stdint.h>
#define MAX_SAMPLING_RAM (24*1024)
//#define MAX_RLE_SAMPLE_COUNT	(64)
#define MAX_RLE_SAMPLE_COUNT	(128)
//#define SAMPLING_FSMC
#define FSMC_ADDR (0x60000000)
//Port A: used for USB_FS, non-usable
//Port B: B2 is BOOT1 pin with 10k pull-up
//Port C: C0 nEnable of STMPS2141STR. C3 output form mic.
//Port D: D5 has pull-up and led, connected nFault of STMPS2141STR. D12-15 connected to color LEDs <- looks safe
//Port E: E0,E1 - active low from LIS302DL, non-usable
#define SAMPLING_PORT GPIOD
//enable manual trigger by user-button
#define SAMPLING_MANUAL
//RLE mode only Enable forcing highest sampled bit to output zero. If disabled - this bit has to be
//zeroed by interrupt handler stealing some cpu cycles.
//#define SAMPLING_RLE_FORCE_ZERO_ON_MSB

void SetBufferSize(uint32_t value);

void SetDelayCount(uint32_t value);

void SetTriggerMask(uint32_t value);

void SetTriggerValue(uint32_t value);

void SetFlags(uint32_t value);

void SetSamplingPeriod(uint32_t value);

