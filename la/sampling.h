#include "main.h"
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
//#define SAMPLING_MANUAL
//RLE mode only Enable forcing highest sampled bit to output zero. If disabled - this bit has to be
//zeroed by interrupt handler stealing some cpu cycles.
//#define SAMPLING_RLE_FORCE_ZERO_ON_MSB
uint32_t triggerMask;
uint32_t triggerValue;
uint16_t flags;
uint16_t period;

int transferSize;

void SetBufferSize(uint32_t);
void SetDelayCount(uint32_t);
void SetTriggerMask(uint32_t);
void SetTriggerValue(uint32_t);
void SetFlags(uint32_t);
void SetSamplingPeriod(uint32_t);

void Stop(void);
void Arm(void);

void Start();
void SetupSamplingTimer(void);
void SetupSamplingDMA(void *, uint32_t);
void SetupDelayTimer(void);
void SetupRegularEXTITrigger(void);

uint32_t CalcDMATransferSize();

void SamplingClearBuffer(void);

int GetBytesPerTransfer(void);

void SamplingComplete(void);
uint32_t ActualTransferCount(void);
uint8_t* GetBufferTail();
uint32_t GetBufferTailSize(void);
uint32_t GetBufferSize(void);
uint8_t* GetBuffer(void);

void PriorityGroupConfig(uint32_t);

// Interrupts

void EnableChannel(IRQn_Type, uint8_t, uint8_t);
void DisableChannel(IRQn_Type);
void SetChannelPriority(IRQn_Type, uint8_t, uint8_t);


//static void PriorityGroupConfig(uint32_t NVIC_PriorityGroup);
