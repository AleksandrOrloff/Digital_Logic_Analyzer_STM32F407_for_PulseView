#include "sump.h"
#include "sampling.h"
#include "usbd_cdc_if.h"


static char metaData[]
     = {SUMP_META_NAME, 'M', 'e', 'a', 's', 'u', 'r', 'i', 'n', 'g', ' ','C', 'o', 'm', 'p', 'l', 'e', 'x', 0,
		SUMP_META_FPGA_VERSION, 'N', 'o', 'F', 'P', 'G', 'A', ' ', ':', '(', 0,
		SUMP_META_CPU_VERSION, 'V', 'e', 'r', 'y', ' ','b' ,'e', 't', 'a', 0,
		SUMP_META_SAMPLE_RATE, BYTE4(maxSampleRate), BYTE3(maxSampleRate), BYTE2(maxSampleRate), BYTE1(maxSampleRate),
		SUMP_META_SAMPLE_RAM, 0, 0, BYTE2(maxSampleMemory), BYTE1(maxSampleMemory), //24*1024 b
		SUMP_META_PROBES_B, 8,
		SUMP_META_PROTOCOL_B, 2,
		SUMP_META_END
};

static uint32_t divider = 1;

uint32_t CalcLocalDivider(uint32_t divider, const uint32_t localFrequency, const uint32_t sumpFrequency)
{
	return localFrequency / (sumpFrequency / (divider + 1)) - 1;
}

int SumpProcessRequest(uint8_t *buffer, uint16_t len)
{
	int result = 0;
	switch(buffer[0])
	{
	case SUMP_CMD_ID://ID
		//APP_FOPS.pIf_DataTx((uint8_t*)"1ALS", 4);
		CDC_Transmit_FS((uint8_t*)"1ALS", 4);
		result = 1;
	  break;
	case SUMP_CMD_META://Query metas
		//APP_FOPS.pIf_DataTx((uint8_t*)metaData, sizeof(metaData));
		CDC_Transmit_FS((uint8_t*)metaData, sizeof(metaData));
		result = 1;
	  break;
	  case SUMP_CMD_SET_SAMPLE_RATE:
		if(len == 5)
		{
			//div120 = 120MHz / (100MHz / (div100 + 1)) - 1;
			divider = *((uint32_t*)(buffer+1));
			//if maximum samplerate is 20MHz => 100/20 = 5, 5 - 1 = 4
			if(divider != 0)
			{
				uint32_t clocks = 168000000; // сделать ф-ю для вычисления clocks
				divider = CalcLocalDivider(divider, clocks, SUMP_ORIGINAL_FREQ);
			}
			if(divider == 0)
			{
				divider = 1;
			}
			SetSamplingPeriod(divider);
			result = 1;
			//GUI_Text(0, 14, (uint8_t*)text, White, Black);
		}
		break;
	  case SUMP_CMD_SET_COUNTS:
		if(len == 5)
		{
			uint16_t readCount  = 1 + *((uint16_t*)(buffer+1));
			uint16_t delayCount = *((uint16_t*)(buffer+3));
			SetBufferSize(4 * readCount);
			SetDelayCount(4 * delayCount);
			result = 1;
			//GUI_Text(100, 14, (uint8_t*)text, White, Black);
		}
		break;
	case SUMP_CMD_SET_BT0_MASK:
		if(len == 5)
		{
			SetTriggerMask(*(uint32_t*)(buffer+1));
			result = 1;
		}
		break;
	case SUMP_CMD_SET_BT0_VALUE:
		if(len == 5)
		{
			SetTriggerValue(*(uint32_t*)(buffer+1));
			result = 1;
		}
		break;
	case SUMP_CMD_SET_FLAGS:
		if(len == 5)
		{
			SetFlags(*(uint16_t*)(buffer+1));
			result = 1;
			
		}
		break;
	}
	return result;
}