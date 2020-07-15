#include "sampling.h"
#include "sump.h"
#include "usbd_cdc_if.h"

static uint32_t transferCount;
static uint32_t delayCount;
uint32_t samplingRam[MAX_SAMPLING_RAM/4];
uint8_t arr[MAX_SAMPLING_RAM];

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

void SetSamplingPeriod(uint32_t value){
    period = value;
    }

void Start()
{
    SetupSamplingTimer();
	SetupSamplingDMA(samplingRam, transferCount);
	SetupDelayTimer();
	SetupRegularEXTITrigger();
	DMA2->HIFCR = DMA_HIFCR_CTCIF5; //No half transfer event on stream 5
	DMA2_Stream5->CR |= DMA_SxCR_EN;
	TIM1->CR1 |= TIM_CR1_CEN;//enable timer
}

void Stop()
{
	DMA2_Stream5->CR &= ~(DMA_SxCR_TCIE | DMA_SxCR_EN);//stop dma
	TIM1->CR1 &= ~TIM_CR1_CEN;//stop sampling timer
}

void Arm()
{
	EXTI->PR = 0xffffffff;//clear pending
	__DSB();
	EXTI->IMR = triggerMask;

	//comletionHandler = handler;
}

void SetupSamplingTimer()
{
	//RCC_APB2PeriphClockCmd(RCC_APB2ENR_TIM1EN, ENABLE);
	__HAL_RCC_TIM1_CLK_ENABLE();
	//Main sampling timer
	TIM1->DIER = 0;
	TIM1->SR &= ~TIM_SR_UIF;
	TIM1->CNT = 0;
	TIM1->PSC = 0;
	TIM1->CR1 = TIM_CR1_URS;
	TIM1->ARR = period;//actual period is +1 of this value
	TIM1->CR2 = 0;
	TIM1->DIER = TIM_DIER_UDE;
	TIM1->EGR = TIM_EGR_UG;
}

void SetupSamplingDMA(void *dataBuffer, uint32_t dataTransferCount)
{
	//RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_DMA2EN, ENABLE);
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	uint32_t dmaSize = CalcDMATransferSize();

	//TIM8->DIER = 0;
	//TIM8->SR &= ~TIM_SR_UIF;

	//TIM1_UP -> DMA2, Ch6, Stream5
	//DMA should be stopped before this point
	DMA2_Stream5->CR = (DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2) | dmaSize | DMA_SxCR_MINC | DMA_SxCR_CIRC;
	//DMA2_Stream5->CR = D<>| dmaSize | DMA_SxCR_MINC | DMA_SxCR_CIRC;
	DMA2_Stream5->M0AR = (uint32_t)dataBuffer;//samplingRam;

	DMA2_Stream5->PAR  = (uint32_t)&(SAMPLING_PORT->IDR);

	DMA2_Stream5->NDTR = dataTransferCount;//transferCount;// / transferSize;
	DMA2_Stream5->FCR = DMA_SxFCR_DMDIS | DMA_SxFCR_FTH; //disable direct mode, set FIFO-threshold
}

void SetupDelayTimer()
{
	//RCC_APB2PeriphClockCmd(RCC_APB2ENR_TIM8EN, ENABLE);
	__HAL_RCC_TIM8_CLK_ENABLE();
	//After-trigger delay timer
	TIM8->CR1 = TIM_CR1_URS;//stop timer too
	TIM8->CNT = 0;
	TIM8->ARR = delayCount;//  / transferSize;
	TIM8->PSC = TIM1->ARR;
	TIM8->CR2 = 0;
	TIM8->EGR = TIM_EGR_UG;
	TIM8->SR &= ~TIM_SR_UIF;
	TIM8->DIER = TIM_DIER_UIE;

	EnableChannel(TIM8_UP_TIM13_IRQn, 2, 0);
	//InterruptController::EnableChannel(TIM8_UP_TIM13_IRQn, 2, 0, SamplingFrameCompelte);
}

void SetupRegularEXTITrigger()
{
	//while (CDC_Transmit_FS((uint8_t *)"tessst", 6) != 0);
	//RCC_APB2PeriphClockCmd(RCC_APB2ENR_SYSCFGEN, ENABLE);
	__HAL_RCC_SYSCFG_CLK_ENABLE();
	//Trigger setup
	uint32_t rising = triggerMask & triggerValue;
	uint32_t falling = triggerMask & ~triggerValue;
	//route exti to triggerMask GPIO port
	uint32_t extiCR = 0;
	switch((uint32_t)SAMPLING_PORT)
	{
	case GPIOA_BASE:extiCR = 0x0000;break;
	case GPIOB_BASE:extiCR = 0x1111;break;
	case GPIOC_BASE:extiCR = 0x2222;break;
	case GPIOD_BASE:extiCR = 0x3333;break;
	case GPIOE_BASE:extiCR = 0x4444;break;
	case GPIOF_BASE:extiCR = 0x5555;break;
	case GPIOG_BASE:extiCR = 0x6666;break;
	case GPIOH_BASE:extiCR = 0x7777;break;
	case GPIOI_BASE:extiCR = 0x8888;break;
	}
	SYSCFG->EXTICR[0] = extiCR;
	SYSCFG->EXTICR[1] = extiCR;
	SYSCFG->EXTICR[2] = extiCR;
	SYSCFG->EXTICR[3] = extiCR;

	EXTI->IMR  = 0;//mask;
	EXTI->PR = 0xffffffff;
	EXTI->RTSR = rising;
	EXTI->FTSR = falling;

	__DSB();
	
	if(triggerMask & 0x0001)EnableChannel(EXTI0_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI0_IRQn);
	if(triggerMask & 0x0002)EnableChannel(EXTI1_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI1_IRQn);
	if(triggerMask & 0x0004)EnableChannel(EXTI2_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI2_IRQn);
	if(triggerMask & 0x0008)EnableChannel(EXTI3_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI3_IRQn);
	if(triggerMask & 0x0010)EnableChannel(EXTI4_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI4_IRQn);
	if(triggerMask & 0x03E0)EnableChannel(EXTI9_5_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
	if(triggerMask & 0xFC00)EnableChannel(EXTI15_10_IRQn, 0, 0);
	else HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);


#ifdef SAMPLING_MANUAL //push-button-trigger
	TIM8->SMCR = TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_2;//External trigger input
	TIM8->SMCR |= TIM_SMCR_SMS_1 | TIM_SMCR_SMS_2;
	TIM8->DIER |= TIM_DIER_TIE;
	EnableChannel(TIM8_TRG_COM_TIM14_IRQn, 2, 0);
	//samplingManualToExternalTransit = interruptHandler;
#endif
}

uint32_t CalcDMATransferSize()
{
	uint32_t dmaSize = 0;
	//handle 8/16/32 bit samplings
	switch(flags & SUMP_FLAG1_GROUPS)
	{
	case SUMP_FLAG1_GR_16BIT:
		transferSize = 2;
		dmaSize = DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0;
		break;
	case SUMP_FLAG1_GR_32BIT:
		transferSize = 4;
		dmaSize = DMA_SxCR_MSIZE_1 | DMA_SxCR_PSIZE_1;
		break;
	case SUMP_FLAG1_GR_8BIT:
	default:
		dmaSize = 0;
		transferSize = 1;
		break;
	}
	return dmaSize;
}

void SamplingClearBuffer()
{
	int i;
	for(i = 0; i < MAX_SAMPLING_RAM / 4; i++)
		samplingRam[i] = 0;
	for(i = 0; i < MAX_SAMPLING_RAM; i++)
		arr[i] = 0;
}

void SamplingComplete()
{
	uint32_t i;
	
	__disable_irq();
	//SUMP requests samples to be sent in reverse order: newest items first
	if(GetBytesPerTransfer() == 1)
	{
		uint8_t* ptr = GetBufferTail() - 1;
		//uint8_t arr[GetBufferSize()];
		for(i = 0; i < GetBufferTailSize(); i++)
		{
			//while (CDC_Transmit_FS((uint8_t*)ptr, 1) != 0){};
			arr[i] = *ptr;
			ptr--;
		}
		ptr = GetBuffer() + GetBufferSize() - 1;
		for(; i < GetBufferSize(); i++)
		{
			//while (CDC_Transmit_FS((uint8_t*)ptr, 1) != 0){};
			arr[i] = *ptr;
			ptr--;
		}
		uint16_t Len_Arr = (uint16_t)GetBufferSize();
		while (CDC_Transmit_FS((uint8_t*)arr, Len_Arr) != 0){};

		//SUMP requests samples to be sent in reverse order: newest items first
		/*for(i = 0; i < GetBufferTailSize(); i++)
		{
			while (CDC_Transmit_FS((uint8_t*)ptr, 1) != 0){};
			ptr--;
		}
		ptr = GetBuffer() + GetBufferSize() - 1;
		for(; i < GetBufferSize(); i++)
		{
			while (CDC_Transmit_FS((uint8_t*)ptr, 1) != 0){};
			ptr--;
		}*/
	}
	else if(GetBytesPerTransfer() == 2)
	{
		uint8_t *ptr = GetBufferTail() - GetBytesPerTransfer();
		for(i = 0; i < GetBufferTailSize(); i += GetBytesPerTransfer())
		{
			CDC_Transmit_FS((uint8_t*)ptr, 2);
			ptr -= GetBytesPerTransfer();
		}
		ptr = GetBuffer() + GetBufferSize() - GetBytesPerTransfer();
		for(; i < GetBufferSize(); i += GetBytesPerTransfer())
		{
			CDC_Transmit_FS((uint8_t*)ptr, 2);
			ptr -= GetBytesPerTransfer();
		}
	}
	__enable_irq();
	
}

uint32_t ActualTransferCount()
{
	return transferCount - (DMA2_Stream5->NDTR & ~3);
}

uint8_t* GetBufferTail()
{
	return (uint8_t*)(samplingRam) + ActualTransferCount() * transferSize;
}

int GetBytesPerTransfer(){return transferSize;}


uint32_t GetBufferTailSize()
{
	return ActualTransferCount() * transferSize;
}

uint32_t GetBufferSize()
{
	return transferCount * transferSize;
}

uint8_t* GetBuffer()
{
	return (uint8_t*)samplingRam;
}


// Interrupts

	void EnableChannel(IRQn_Type irqChannel, uint8_t priority, uint8_t subpriority)
	{
		//DisableChannel(irqChannel);
		HAL_NVIC_DisableIRQ(irqChannel);
		//SetHandler(irqChannel, handler);
		//SetChannelPriority(irqChannel, priority, subpriority);
		HAL_NVIC_SetPriority(irqChannel, 0, 0);
		//NVIC->ISER[irqChannel >> 0x05] =
	      //(uint32_t)0x01 << (irqChannel & (uint8_t)0x1F);
		HAL_NVIC_EnableIRQ(irqChannel);

	}