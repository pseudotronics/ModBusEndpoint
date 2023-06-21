#include <stdint.h>
#include <stdbool.h>
#include <config.h>
#include "bsp_functions.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include "mb.h"

static uint8_t s_input_state = 0;

static bool s_output_state[NUM_OUTPUTS] = { false };
static volatile bool s_changed = false;
static uint8_t s_debounce[2] = { 0, 0 };

static uint32_t s_bsp_counters[2] = { 0 };

enum BSP_COUNTERS
{
	OUTPUT_1,
	OUTPUT_2
};

void bsp_setup_pins()
{
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
	gpio_put(LED_PIN, true);
	
	gpio_init(INPUT_1_PIN);
	gpio_set_dir(INPUT_1_PIN, GPIO_IN);
	gpio_set_input_enabled(INPUT_1_PIN, true);
	
	gpio_init(INPUT_2_PIN);
	gpio_set_dir(INPUT_2_PIN, GPIO_IN);
	gpio_set_input_enabled(INPUT_2_PIN, true);
	
	irq_set_enabled(IO_IRQ_BANK0, true);
	
	s_input_state |= gpio_get(INPUT_1_PIN) ? 0x01 : 0;
	s_input_state |= gpio_get(INPUT_2_PIN) ? 0x02 : 0;
	
	gpio_init(OUTPUT_1_PIN);
	gpio_set_dir(OUTPUT_1_PIN, GPIO_OUT);
	
	gpio_init(OUTPUT_2_PIN);
	gpio_set_dir(OUTPUT_2_PIN, GPIO_OUT);
	
	gpio_init(ADDRESS_B0_PIN);
	gpio_init(ADDRESS_B1_PIN);
	gpio_init(ADDRESS_B2_PIN);
	gpio_init(ADDRESS_B3_PIN);
	gpio_init(ADDRESS_B4_PIN);
	gpio_init(ADDRESS_B5_PIN);
	gpio_init(ADDRESS_B6_PIN);
	gpio_init(ADDRESS_B7_PIN);
	
	gpio_set_dir(ADDRESS_B0_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B1_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B2_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B3_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B4_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B5_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B6_PIN, GPIO_IN);
	gpio_set_dir(ADDRESS_B7_PIN, GPIO_IN);
	
}

bool get_LED_state()
{
	return !gpio_get(LED_PIN);
}
void set_LED_state(bool state)
{
	gpio_put(LED_PIN, !state);
} 

bool get_output(uint8_t channel)
{
	if (channel == 1)
	{
		return gpio_get_out_level(OUTPUT_1_PIN);
	}
	else if (channel == 2)
	{
		return gpio_get_out_level(OUTPUT_2_PIN);
	}
	return false;
}

void pulse_output(uint8_t channel)
{
	#define PULSE_TIME_MS 100
	if (channel == 0)
	{
		gpio_put(OUTPUT_1_PIN, true);
		sleep_ms(PULSE_TIME_MS);
		gpio_put(OUTPUT_1_PIN, false);
		s_bsp_counters[OUTPUT_1]++;
	}
	else if (channel == 1)
	{
		gpio_put(OUTPUT_2_PIN, true);
		sleep_ms(PULSE_TIME_MS);
		gpio_put(OUTPUT_2_PIN, false);
		s_bsp_counters[OUTPUT_2]++;
	}
	else
	{
		return;
	}
}

bool get_input(uint8_t channel)
{
	return s_input_state & (1 << channel);
}

uint8_t get_address_byte()
{
	return (uint8_t)((gpio_get_all() >> 16) & 0xFF);
}

float get_bus_voltage();

bool input_changed()
{
	if (s_changed)
	{
		s_changed = false;
		return true;
	}
	return false;
}

void update_inputs()
{
	s_debounce[0] <<= 1;
	s_debounce[0] |= gpio_get(INPUT_1_PIN);
	
	s_debounce[1] <<= 1;
	s_debounce[1] |= gpio_get(INPUT_2_PIN);
	
	if (s_debounce[0] == 0xFF && !(s_input_state & 0x01))
	{
		s_input_state |= 0x01;
		mb_set_discrete_input(0, true);
		s_changed = true;
	}
	else if (s_debounce[0] == 0x00 && s_input_state & 0x01)
	{
		s_input_state &= ~(0x01);
		mb_set_discrete_input(0, false);
		s_changed = true;
	}
	
	if (s_debounce[1] == 0xFF && !(s_input_state & 0x02))
	{
		s_input_state |= 0x02;
		mb_set_discrete_input(1, true);
		s_changed = true;
	}
	else if (s_debounce[1] == 0x00 && s_input_state & 0x02)
	{
		s_input_state &= ~(0x02);
		mb_set_discrete_input(1, false);
		s_changed = true;
	}
}

void update_outputs()
{
	for (int i = 0; i < NUM_OUTPUTS; i++)
	{
		bool coil_value = mb_get_coil(i);
		if (coil_value)
		{
			pulse_output(i);
			mb_set_coil(i, false);
		}
	}
}

void print_bsp_stats()
{
	printf("** BSP STATISTICS **\r\n");
	printf("OUTPUT 1 CYCLES\t= %lu\r\n", s_bsp_counters[OUTPUT_1]);
	printf("OUTPUT 2 CYCLES\t= %lu\r\n", s_bsp_counters[OUTPUT_2]);
}