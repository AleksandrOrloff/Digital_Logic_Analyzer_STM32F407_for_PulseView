#include "sampling.h"
#include "sump.h"

static uint32_t transferCount;
static uint32_t delayCount;
uint32_t triggerMask;
uint32_t triggerValue;
uint16_t flags;
uint16_t period;
uint32_t samplingRam[MAX_SAMPLING_RAM/4];
int transferSize;

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
	DMA2->HIFCR = DMA_HIFCR_CTCIF5;
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
	__HAL_RCC_DMA2_CLK_ENABLE();
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
	DMA2_Stream5->FCR = DMA_SxFCR_DMDIS | DMA_SxFCR_FTH;
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

	//InterruptController::EnableChannel(TIM8_UP_TIM13_IRQn, 2, 0, SamplingFrameCompelte);
}

void SetupRegularEXTITrigger()
{
	//RCC_APB2PeriphClockCmd(RCC_APB2ENR_SYSCFGEN, ENABLE);

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
	else DisableChannel(EXTI0_IRQn);
	if(triggerMask & 0x0002)EnableChannel(EXTI1_IRQn, 0, 0);
	else DisableChannel(EXTI1_IRQn);
	if(triggerMask & 0x0004)EnableChannel(EXTI2_IRQn, 0, 0);
	else DisableChannel(EXTI2_IRQn);
	if(triggerMask & 0x0008)EnableChannel(EXTI3_IRQn, 0, 0);
	else DisableChannel(EXTI3_IRQn);
	if(triggerMask & 0x0010)EnableChannel(EXTI4_IRQn, 0, 0);
	else DisableChannel(EXTI4_IRQn);
	if(triggerMask & 0x03E0)EnableChannel(EXTI9_5_IRQn, 0, 0);
	else DisableChannel(EXTI9_5_IRQn);
	if(triggerMask & 0xFC00)EnableChannel(EXTI15_10_IRQn, 0, 0);
	else DisableChannel(EXTI15_10_IRQn);

#ifdef SAMPLING_MANUAL //push-button-trigger
	TIM8->SMCR = TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_2;//External trigger input
	TIM8->SMCR |= TIM_SMCR_SMS_1 | TIM_SMCR_SMS_2;
	TIM8->DIER |= TIM_DIER_TIE;
	InterruptController::EnableChannel(TIM8_TRG_COM_TIM14_IRQn, 2, 0, SamplingManualStart);
	samplingManualToExternalTransit = interruptHandler;
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
	for(int i = 0; i < MAX_SAMPLING_RAM; i++)
		samplingRam[i] = 0;
}



// Interrupts

	//Âêëþ÷àåò ïðåðûâàíèå, çàäàåò ïðèîðèòåò è óñòàíàâëèâàåò îáðàáî÷èê
	static void EnableChannel(IRQn_Type irqChannel, uint8_t priority, uint8_t subpriority)
	{
		DisableChannel(irqChannel);
		//SetHandler(irqChannel, handler);
		SetChannelPriority(irqChannel, priority, subpriority);
		NVIC->ISER[irqChannel >> 0x05] =
	      (uint32_t)0x01 << (irqChannel & (uint8_t)0x1F);

	}
	//Îòêëþ÷àåò ïðåðûâàíèå
	static void DisableChannel(IRQn_Type irqChannel)
	{
	    NVIC->ICER[irqChannel >> 0x05] =
	      (uint32_t)0x01 << (irqChannel & (uint8_t)0x1F);
	}
	static void SetChannelPriority(IRQn_Type irqChannel, uint8_t priority, uint8_t subpriority)
	{
	#ifdef STM32L1XX
		NVIC->IP[(uint32_t)(irqChannel)] = ((priority << (8 - __NVIC_PRIO_BITS)) & 0xff);
	#else
		uint32_t tmppriority = 0x00, tmppre = 0x00, tmpsub = 0x0F;
		/* Compute the Corresponding IRQ Priority --------------------------------*/
		tmppriority = (0x700 - ((SCB->AIRCR) & (uint32_t)0x700))>> 0x08;
		tmppre = (0x4 - tmppriority);
		tmpsub = tmpsub >> tmppriority;

		tmppriority = (uint32_t)priority << tmppre;
		tmppriority |=  subpriority & tmpsub;
		tmppriority = tmppriority << 0x04;
		NVIC->IP[irqChannel] = tmppriority;
	#endif
	}
	
	#define AIRCR_VECTKEY_MASK		((uint32_t)0x05FA0000)
	#define AIRCR_SYSRESETREQ		((uint32_t)0x00000004)

	static void PriorityGroupConfig(uint32_t NVIC_PriorityGroup)
	{
		SCB->AIRCR = AIRCR_VECTKEY_MASK | NVIC_PriorityGroup;
	}
	static void SystemReset()
	{
		SCB->AIRCR = AIRCR_VECTKEY_MASK | AIRCR_SYSRESETREQ;
	}