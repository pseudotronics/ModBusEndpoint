#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cli_defs.h"

extern cmd_t cmds[];
extern cli_status_t rslt;
extern cli_t cli;

void cli_println(const char * string);

#ifdef __cplusplus
}
#endif