#pragma once

#ifdef __cplusplus
extern "C" {
#endif

	#include "cli_defs.h"

	cli_status_t cli_process();
	void cli_init();
	void cli_put(char c);

	extern cli_t cli;
	
#ifdef __cplusplus
}
#endif

