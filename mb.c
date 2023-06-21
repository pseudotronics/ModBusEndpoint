#include "mb.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "crc.h"
#include "bsp_functions.h"

static const uint16_t MB_INTER_CHARACTER_DELAY = 750;
static const uint16_t MB_INTER_FRAME_DELAY = 1750;

static uint8_t s_input_buffer[MB_BUFFER_SIZE];
static uint16_t s_input_buffer_count = 0;
static uint8_t s_output_buffer[MB_BUFFER_SIZE];
static uint16_t s_output_buffer_count = 0;
static uint8_t s_address = 0;
static volatile enum MB_STATES s_mb_state = MB_INIT;
static enum FRAME_STATUS s_mb_frame_status = MB_FRAME_OK;
static uint64_t s_last_byte_us = 0;
static uint16_t s_mb_serial_counters[8] = { 0 };
static uart_hw_t *s_device;

#if MB_INPUTS
static uint16_t s_mb_inputs[MB_INPUTS / 16 + (MB_INPUTS % 16 ? 1 : 0)] = { 0 };
#endif

#if MB_COILS
static uint16_t s_mb_coils[MB_COILS / 16 + (MB_COILS % 16 ? 1 : 0)] = { 0 };
#endif

#if MB_INPUT_REGISTERS
static uint16_t s_mb_input_regsiters[MB_INPUT_REGISTERS] = { 0 };			  
#endif 

#if MB_HOLDING_REGISTERS
static uint16_t s_mb_holding_regsiters[MB_HOLDING_REGISTERS] = { 0 };		
#endif



enum MB_COUNTERS
{
	MB_BUS_MESSAGE,
	MB_BUS_COM_ERROR,
	MB_EXCEPTION,
	MB_MESSAGE,
	MB_NO_RESPONSE,
	MB_NAK,
	MB_BUSY,
	MB_OVERRUN
};

void mb_init(uint8_t address)
{
	s_address = address;
	s_device = uart_get_hw(RS485_DEV);
	gpio_set_function(RS485_TX_PIN, GPIO_FUNC_UART);
	gpio_set_function(RS485_RX_PIN, GPIO_FUNC_UART);
	
	gpio_init(RS485_TX_EN_PIN);
	gpio_set_dir(RS485_TX_EN_PIN, GPIO_OUT);
	gpio_init(RS485_RX_EN_PIN);
	gpio_set_dir(RS485_RX_EN_PIN, GPIO_OUT);
	
	uint baud = uart_init(RS485_DEV, RS485_BAUD);
	#if MB_DEBUG_ENABLE
		printf("Baud Actual: %d", baud);
	#endif // MB_DEBUG_ENABLE == 1
	uart_set_format(RS485_DEV, RS485_DATA_BITS, RS485_STOP_BITS, RS485_PARITY);
	uart_set_hw_flow(RS485_DEV, false, false);
	uart_set_fifo_enabled(RS485_DEV, true);
	
	s_last_byte_us = time_us_64();
	while (uart_is_readable_within_us(RS485_DEV, MB_INTER_FRAME_DELAY))
	{
		uart_getc(RS485_DEV); //drop chars until bus is idle for T3.5
	}
	
	irq_set_exclusive_handler(RS485_IRQ, mb_receive_char);
	irq_set_enabled(RS485_IRQ, true);
	uart_set_irq_enables(RS485_DEV, true, false);
	s_mb_state = MB_IDLE;
}

void mb_set_id(uint8_t address)
{
	s_address = address;
}

void mb_receive_char()
{
	if (s_mb_state == MB_IDLE)
	{
		s_input_buffer_count = 0; 
		s_mb_frame_status = MB_FRAME_OK;
		s_mb_state = MB_RECEPTION;
		while (uart_is_readable_within_us(RS485_DEV, MB_INTER_CHARACTER_DELAY))
		{
			if (s_input_buffer_count < MB_BUFFER_SIZE)
			{
				s_input_buffer[s_input_buffer_count++] = uart_getc(RS485_DEV);
			}
			else
			{
				uart_getc(RS485_DEV);
				s_mb_frame_status = MB_FRAME_NOK; // overrun error
				s_mb_serial_counters[MB_OVERRUN]++;
			}
			s_last_byte_us = time_us_64();
		}
		s_mb_state = MB_WAITING;
	}
	else if (s_mb_state == MB_WAITING || s_mb_state == MB_PROCESSING_RESPONSE || s_mb_state == MB_PROCESSING_NO_RESPONSE)
	{
		// incomplete frame discard until idle
		while (uart_is_readable(RS485_DEV))
		{
			uart_getc(RS485_DEV);
			s_mb_frame_status = MB_FRAME_NOK;
			s_last_byte_us = time_us_64();
		}	
	}
	
	if (s_device->dr & (UART_UARTDR_PE_BITS | UART_UARTDR_FE_BITS | UART_UARTDR_OE_BITS))
	{
		s_mb_frame_status = MB_FRAME_NOK; // mark frame bad
		s_device->rsr = 0; // clear error
	}
}

void mb_process()
{
	uint64_t diffTime = time_us_64() - s_last_byte_us;
	switch (s_mb_state)
	{
	case MB_WAITING:
		if (s_mb_frame_status != MB_FRAME_OK || s_input_buffer_count <= 3)
		{
			s_mb_serial_counters[MB_BUS_COM_ERROR]++;
			s_mb_state = MB_DISCARD;
			#if MB_DEBUG_ENABLE		
				printf("FRAME NOT OK!\r\n");
				printf("Frame (Size = %d):\r\n", s_input_buffer_count);		
				for (uint8_t i = 0; i < s_input_buffer_count; i++)
				{
					printf("%02X ", s_input_buffer[i]);
				}
				printf("\r\n");		 	
			#endif // MB_DEBUG_ENABLE == 1
			
			break;
		}
		// FRAME OK, LENGTH OK
		uint16_t frame_crc = ((uint16_t)s_input_buffer[s_input_buffer_count - 1]) << 8;
		frame_crc |= s_input_buffer[s_input_buffer_count - 2];

		if (frame_crc != CRC16(s_input_buffer, s_input_buffer_count - 2))
		{
			s_mb_serial_counters[MB_BUS_COM_ERROR]++;
			s_mb_state = MB_DISCARD;
			#if MB_DEBUG_ENABLE	
				printf("CRC NOT OK!");
			#endif // MB_DEBUG_ENABLE == 1
			break;
		}
		//CRC OK
		
		s_mb_serial_counters[MB_BUS_MESSAGE]++;
		
		if (s_input_buffer[0] != s_address && s_input_buffer[0] != 0)
		{
			// MSG NOT FOR ME
			s_mb_state = MB_DISCARD;
			break;
		}

		// MSG FOR ME
		s_mb_serial_counters[MB_MESSAGE]++;
		if (s_input_buffer[0] == 0)
		{
			s_mb_serial_counters[MB_NO_RESPONSE]++;
			s_mb_state = MB_PROCESSING_NO_RESPONSE;
			break;
		}
		s_mb_state = MB_PROCESSING_RESPONSE;
		break;
		// \/ Intentional Fall Through \/
	case MB_PROCESSING_RESPONSE:	
	case MB_PROCESSING_NO_RESPONSE:
	case MB_DISCARD:
		if (s_mb_frame_status == MB_FRAME_NOK)
			s_mb_state = MB_DISCARD;
		
		if (diffTime < MB_INTER_FRAME_DELAY)
			break;
		
		if (s_mb_state == MB_DISCARD)
		{
			s_mb_state = MB_IDLE;
			break;
		}
		
		mb_function_process();
		
		//s_input_buffer_count = 0; // Discard Packet
		if (s_mb_state == MB_PROCESSING_NO_RESPONSE)
		{
			s_mb_state = MB_IDLE;
			return;
		}
		
		s_mb_state = MB_EMISSION;
		break;
		
	case MB_EMISSION:
		
		#if MB_DEBUG_ENABLE	
			printf("Frame (Size = %d):\r\n", s_input_buffer_count);		
			for (uint8_t i = 0; i < s_input_buffer_count; i++)
			{
				printf("%02X ", s_input_buffer[i]);
			}
			printf("\r\n");
		#endif // MB_DEBUG_ENABLE == 1
		
		mb_tx_enable();
		uart_write_blocking(RS485_DEV, s_output_buffer, (size_t)s_output_buffer_count);
		uart_tx_wait_blocking(RS485_DEV);
		s_output_buffer_count = 0;
		mb_tx_disable();
		
		s_mb_state = MB_IDLE;
		break;
		
	default:
		break;
	}
}

void mb_function_process()
{
	uint8_t mb_function = s_input_buffer[1];
	uint16_t mb_mem_address;
	switch (mb_function)
	{
	#if MB_INPUTS
	case MB_FUNC_READ_DISCRETE_INPUTS:
		if (s_input_buffer_count != 8)
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_DATA);	
			break;
		}
		
		mb_mem_address = mb_parse_addr();
		if (mb_mem_address >= MB_INPUTS) // check memory address
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_ADDRESS);
			break;
		} 
		
		uint16_t inputs_to_read = ((uint16_t)s_input_buffer[4]) << 8;
		inputs_to_read |= s_input_buffer[5];
		#if MB_DEBUG_ENABLE
			printf("Start Addr %d\r\n", mb_mem_address);
			printf("Count Read: %d\r\n", inputs_to_read);
		#endif // MB_DEBUG_ENABLE == 1
		
		if (mb_mem_address + inputs_to_read > MB_INPUTS)
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_DATA);
		}
		
		
		memcpy(s_output_buffer, s_input_buffer, 2);
		s_output_buffer_count = 2;
		
		uint8_t remainder_mask_size = inputs_to_read % 8;
		uint8_t byte_count = inputs_to_read / 8 + (inputs_to_read ? 1 : 0);
		s_output_buffer[s_output_buffer_count++] = byte_count;
		memset(s_output_buffer + s_output_buffer_count, 0, byte_count);
		for (uint16_t i = 0; i < inputs_to_read; i++)
		{
			if (i > 0 && (i % 8 == 0))
			{
				s_output_buffer_count++;
			}
			s_output_buffer[s_output_buffer_count] |= mb_get_discrete_input(i) << (i % 8);
		}
		s_output_buffer_count++;
		mb_add_crc();
		break;
	#endif
		
	#if MB_COILS
	case MB_FUNC_WRITE_SINGLE_COIL:	
		if (s_input_buffer_count != 8)
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_DATA);	
			break;
		}
		
		mb_mem_address = mb_parse_addr();
		if (mb_mem_address >= MB_COILS) // check memory address
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_ADDRESS);
			break;
		}
		
		uint16_t output_value = ((uint16_t)s_input_buffer[4]) << 8;
		output_value |= s_input_buffer[5];
		
		#if MB_DEBUG_ENABLE
			printf("Output Value: %04X\r\n", output_value);
		#endif // MB_DEBUG_ENABLE == 1
		
		if (output_value == 0x0000)
		{
			mb_set_coil(mb_mem_address, false);
		}
		else if (output_value == 0xFF00)
		{
			mb_set_coil(mb_mem_address, true);
		}
		else
		{
			mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_DATA);
			break;
		}
		
		memcpy(s_output_buffer, s_input_buffer, s_input_buffer_count);
		s_output_buffer_count = s_input_buffer_count;
		
		break;
	#endif
		
	#if MB_INPUT_REGISTERS
		
	#endif
		
	#if MB_HOLDING_REGISTERS
		
	#endif
	default:
		mb_set_output_as_error(MB_EXCEPTION_ILLEGAL_FUNCTION);
		break;
	}
	
}

void mb_add_crc()
{
	uint16_t crc = CRC16(s_output_buffer, s_output_buffer_count);
	s_output_buffer[s_output_buffer_count] = crc & 0xFF;
	s_output_buffer[s_output_buffer_count + 1] = (crc >> 8) & 0xFF;
	s_output_buffer_count += 2;
}

void mb_tx_enable()
{
	//disable RX IRQ, disable RX, clear FIFO of garbage
	irq_set_enabled(RS485_IRQ, false);
	gpio_put(RS485_RX_EN_PIN, true);
	gpio_put(RS485_TX_EN_PIN, true);
	set_LED_state(true);
	while (!gpio_get(RS485_TX_EN_PIN)) { tight_loop_contents(); }
}

void mb_tx_disable()
{
	gpio_put(RS485_RX_EN_PIN, false);
	gpio_put(RS485_TX_EN_PIN, false);
	while (gpio_get(RS485_RX_EN_PIN)) { tight_loop_contents(); }
	set_LED_state(false);
	while (uart_is_readable(RS485_DEV))
	{
		uart_getc(RS485_DEV);
	}
	s_device->rsr = 0; // reset any error
	irq_set_enabled(RS485_IRQ, true);
}

uint16_t mb_parse_addr()
{
	uint16_t addr = ((uint16_t)s_input_buffer[2]) << 8;
	addr |= s_input_buffer[3];
	return addr;
}

void mb_set_output_as_error(uint8_t error)
{
	s_output_buffer[0] = s_address;
	s_output_buffer[1] = s_input_buffer[1] + MB_FUNC_EXCEPTION_MODIFIER;
	s_output_buffer[2] = error;
	s_output_buffer_count = 3;
	mb_add_crc();
}

void mb_print_stats()
{
	printf("** MODBUS STATISTICS **\r\n");
	printf("BUS MESSAGE\t= %d\r\n", s_mb_serial_counters[MB_BUS_MESSAGE]);
	printf("BUS COM ERROR\t= %d\r\n", s_mb_serial_counters[MB_BUS_COM_ERROR]);
	printf("EXCEPTION\t= %d\r\n", s_mb_serial_counters[MB_EXCEPTION]);
	printf("MESSAGE\t\t= %d\r\n", s_mb_serial_counters[MB_MESSAGE]);
	printf("NO RESPONSE\t= %d\r\n", s_mb_serial_counters[MB_NO_RESPONSE]);
	printf("NAK\t\t= %d\r\n", s_mb_serial_counters[MB_NAK]);
	printf("BUSY\t\t= %d\r\n", s_mb_serial_counters[MB_BUSY]);
	printf("OVERRUN\t\t= %d\r\n", s_mb_serial_counters[MB_OVERRUN]);
}

#if MB_COILS
void mb_set_coil(uint16_t addr, bool on)
{
	uint16_t register_addr = addr / 16;
	uint16_t bit_mask = 1  << (addr % 16);
	if (on)
	{
		s_mb_coils[register_addr] |= bit_mask;
	}
	else
	{
		s_mb_coils[register_addr] &= ~bit_mask;
	}
}

bool mb_get_coil(uint16_t addr)
{
	uint16_t register_addr = addr / 16;
	uint16_t bit_mask = 1  << (addr % 16);
	return s_mb_coils[register_addr] & bit_mask;
}
#endif

#if MB_INPUTS
void mb_set_discrete_input(uint16_t addr, bool on)
{
	uint16_t register_addr = addr / 16;
	uint16_t bit_mask = 1  << (addr % 16);
	if (on)
	{
		s_mb_inputs[register_addr] |= bit_mask;
	}
	else
	{
		s_mb_inputs[register_addr] &= ~bit_mask;
	}
}

bool mb_get_discrete_input(uint16_t addr)
{
	uint16_t register_addr = addr / 16;
	uint16_t bit_mask = 1  << (addr % 16);
	return s_mb_inputs[register_addr] & bit_mask;
}
#endif