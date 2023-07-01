#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "cli.h"
#include "ring_buffer.hpp"
#include "config.h"
#include "bsp_functions.h"
#include "mb.h"

uint8_t mb_address = 0;

int main() {
	stdio_init_all(); 
	
	bsp_setup_pins();
	
	mb_address = get_address_byte();
	printf("Address: 0x%02x\r\n", mb_address);
	mb_init(mb_address);
	cli_init();
	while (1) {
		cli_process();
		update_inputs();
		mb_process();
		update_outputs();
		light_update();
	}
}

