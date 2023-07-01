#pragma once

#include <stdint.h>
#include "pico/stdlib.h"

#define MB_DEBUG_ENABLE 1

#define MB_BROADCAST_ID 0
	
#define MB_FUNC_READ_DISCRETE_INPUTS 0x02
#define MB_FUNC_READ_COILS 0x01
#define MB_FUNC_WRITE_SINGLE_COIL 0x05
#define MB_FUNC_WRITE_MULTIPLE_COILS 0x0F
#define MB_FUNC_READ_INPUT_REGISTER 0x04
#define MB_FUNC_READ_HOLDING_REGISTERS 0x03
#define MB_FUNC_WRITE_SINGLE_REGISTER 0x06
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS 0x10
#define MB_FUNC_READ_WRITE_MULTIPLE_REGISTERS 0x17
#define MB_FUNC_MASK_WRITE_REGISTER 0x16
#define MB_FUNC_READ_FIFO_QUEUE 0x18
#define MB_FUNC_READ_FILE_RECORD 0x14
#define MB_FUNC_WRITE_FILE_RECORD 0x15
#define MB_FUNC_READ_EXCEPTION_STATUS 0x07
#define MB_FUNC_DIAGNOSTICS 0x08
#define MB_FUNC_GET_COM_EVENT_COUNTER 0x0B
#define MB_FUNC_GET_COM_EVENT_LOG 0x0C
#define MB_FUNC_REPORT_SERVER_ID 0x11
#define MB_FUNC_READ_DEVICE_ID 0x2B

#define MB_DIAG_SUB_QUERY_DATA 0x00
#define MB_DIAG_RESTART_COM 0x01
#define MB_DIAG_RETURN_DIAG 0x02
#define MB_DIAG_CHANGE_ASCII_DEL 0x03
#define MB_DIAG_FORCE_LISTEN 0x04
#define MB_DIAG_CLEAR 0x0A
#define MB_DIAG_MSG_COUNT 0x0B
#define MB_DIAG_COM_ERROR_COUNT 0x0C
#define MB_DIAG_EXCEPT_COUNT 0x0D
#define MB_DIAG_SERVER_MSG_COUNT 0x0E
#define MB_DIAG_SERVER_NO_RESP_COUNT 0x0F
#define MB_DIAG_SERVER_NAK_COUNT 0x10
#define MB_DIAG_SERVER_BUSY_COUNT 0x11
#define MB_DIAG_BUS_OVERRUN_COUNT 0x12

#define MB_EXCEPTION_ILLEGAL_FUNCTION 0x01
#define MB_EXCEPTION_ILLEGAL_ADDRESS 0x02
#define MB_EXCEPTION_ILLEGAL_DATA 0x03
#define MB_EXCEPTION_DEVICE_FAILURE 0x04
#define MB_EXCEPTION_ACK 0x05
#define MB_EXCEPTION_BUSY 0x06
#define MB_EXCEPTION_MEM_PARITY_ERROR 0x08

#define MB_FUNC_EXCEPTION_MODIFIER 0x80

#define MB_BUFFER_SIZE 256 // max frame size

// Data Model Definitions
#define MB_INPUTS 2
#define MB_COILS 1
#define MB_INPUT_REGISTERS 0
#define MB_HOLDING_REGISTERS 0

// Peripheral Definitions
#define RS485_TX_PIN 0
#define RS485_RX_PIN 1
#define RS485_TX_EN_PIN 2
#define RS485_RX_EN_PIN 3
	
#define RS485_DEV uart0
#define RS485_IRQ UART0_IRQ

#define RS485_BAUD 115200
#define RS485_DATA_BITS 8
#define RS485_STOP_BITS 1
#define RS485_PARITY UART_PARITY_EVEN
#define RS485_SYM_SIZE (1 + RS485_DATA_BITS + RS485_STOP_BITS + 1)

#ifdef __cplusplus
extern "C" {
#endif
	
	enum MB_STATES
	{
		MB_INIT,
		MB_IDLE,
		MB_RECEPTION,
		MB_WAITING,
		MB_EMISSION,
		MB_PROCESSING_RESPONSE,
		MB_PROCESSING_NO_RESPONSE,
		MB_DISCARD
	};
	
	enum FRAME_STATUS
	{
		MB_FRAME_OK,
		MB_FRAME_NOK
	};
	
	void mb_init(uint8_t address);
	void mb_set_id(uint8_t address);
	void mb_receive_char();
	void mb_process();
	void mb_function_process();
	void mb_add_crc();
	void mb_tx_enable();
	void mb_tx_disable();
	uint16_t mb_parse_addr();
	bool mb_valid_addr(uint16_t addr);
	void mb_set_output_as_error(uint8_t error);
	void mb_print_stats();
	
#if MB_COILS
	void mb_set_coil(uint16_t addr, bool on);
	bool mb_get_coil(uint16_t addr);
#endif

#if MB_INPUTS
	void mb_set_discrete_input(uint16_t addr, bool on);
	bool mb_get_discrete_input(uint16_t addr);
#endif
	


	#ifdef __cplusplus
	}
#endif