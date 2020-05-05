#include "sump.h"
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
	}
	return result;
}