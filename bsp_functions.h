#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUM_OUTPUTS 2

#ifdef __cplusplus
extern "C" {
#endif
	
	void bsp_setup_pins();

	bool get_LED_state();
	void set_LED_state(bool state);

	void pulse_output(uint8_t channel);

	bool get_input(uint8_t channel);

	uint8_t get_address_byte();

	float get_bus_voltage();
	
	bool input_changed();
	
	void update_inputs();
	
	void update_outputs();
	
	void print_bsp_stats();

#ifdef __cplusplus
}
#endif