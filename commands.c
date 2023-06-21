#include "commands.h"
#include "cli.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "config.h"
#include <stdlib.h>
#include "bsp_functions.h"
#include "build_number.h"
#include "build_version.h"
#include "mb.h"

static cli_status_t cli_cmd_led(int argc, char **argv);
static cli_status_t cli_cmd_input(int argc, char **argv);
static cli_status_t cli_cmd_output(int argc, char **argv);
static cli_status_t cli_cmd_id(int argc, char **argv);
static cli_status_t cli_cmd_stats(int argc, char **argv);
static cli_status_t cli_cmd_version(int argc, char **argv);


cmd_t cmds[] =
{
	{
		.cmd = "led",
		.func = cli_cmd_led,
		.help = "<on/off/toggle> (Sets the built-in LED to the supplied state)"
	},
	{
		.cmd = "inputs",
		.func = cli_cmd_input,
		.help = "(Returns the state of the discrete inputs)"
	},
	{
		.cmd = "pulse",
		.func = cli_cmd_output,
		.help = "<output channel 1-2> (Pulses the selected output for 100ms)"
	},
	{
		.cmd = "id",
		.func = cli_cmd_id,
		.help = "(Returns the ModBus ID of the board, updates running value if changed)"
	},
	{
		.cmd = "stats",
		.func = cli_cmd_stats,
		.help = "(Returns the board statistics since last reset)"
	},
	{
		.cmd = "version",
		.func = cli_cmd_version,
		.help = "(Returns the Build Versions & Number)"
	}
};

cli_t cli =
{
	.println = cli_println,
	.cmd_tbl = cmds,
	.cmd_cnt = sizeof(cmds)	/ sizeof(cmd_t)
};

void cli_println(const char * string)
{
	puts(string);
}

static cli_status_t cli_cmd_led(int argc, char **argv)
{
	if (argc != 2)
		return CLI_E_INVALID_ARGS;
	
	if (!strncmp(argv[1], "on", 2))
	{
		gpio_put(LED_PIN, false);
		puts("LED[ON]");
	}
	else if (!strncmp(argv[1], "off", 3))
	{
		gpio_put(LED_PIN, true);
		puts("LED[OFF]");
	}
	else if (!strncmp(argv[1], "toggle", 6))
	{
		bool state = gpio_get(LED_PIN);
		gpio_put(LED_PIN, !state);
		puts(state ? "LED[ON]" : "LED[OFF]");
	}
	
	return CLI_OK;
}

static cli_status_t cli_cmd_input(int argc, char **argv)
{
	printf("[1] = %s\r\n", get_input(0) ? "TRUE" : "FALSE");
	printf("[2] = %s\r\n", get_input(1) ? "TRUE" : "FALSE");
	return CLI_OK;
}

static cli_status_t cli_cmd_output(int argc, char **argv)
{
	if (argc != 2)
		return CLI_E_INVALID_ARGS;

	uint8_t channel = atoi(argv[1]);

	if (channel == 0 || channel > NUM_OUTPUTS) // Is valid 1 -> NUM_OUTPUTS
		return CLI_E_INVALID_ARGS;

	pulse_output(channel - 1);
	
	return CLI_OK;
}

static cli_status_t cli_cmd_id(int argc, char **argv)
{
	uint8_t address = get_address_byte();
	printf("Modbus Address: 0x%02x\r\n", address);
	mb_set_id(address);
	return CLI_OK;
}

static cli_status_t cli_cmd_stats(int argc, char **argv)
{
	mb_print_stats();
	printf("\r\n");
	print_bsp_stats();
	return CLI_OK;
}

static cli_status_t cli_cmd_version(int argc, char **argv)
{
	printf("Build Information: %d.%d.%d\r\n", BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR, BUILD_NUMBER);
}

