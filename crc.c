#include "crc.h"

uint16_t CRC16(uint8_t *msg, uint16_t data_len)
{
	uint8_t crc_hi = 0xFF; /* high byte of CRC initialized */
	uint8_t crc_low = 0xFF; /* low byte of CRC initialized */
	unsigned uIndex; /* will index into CRC lookup table */
	while (data_len--) /* pass through message buffer */
	{
		uIndex = crc_low ^ *msg++; /* calculate the CRC */
		crc_low = crc_hi ^ auchCRCHi[uIndex];
		crc_hi = auchCRCLo[uIndex];
	}
	return (crc_hi << 8 | crc_low) ;
} 