#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "cli.h"

#define MAX_BUFFER_SIZE 80
#define CMD_TERMINATOR '\r'

static uint8_t buffer[MAX_BUFFER_SIZE + 1];
static uint8_t *buffer_ptr;

#define MAX_HISTORY 10
static uint8_t s_History[MAX_HISTORY][MAX_BUFFER_SIZE];
static int s_HistoryIndex = 0;

static uint8_t cmd_buffer[MAX_BUFFER_SIZE];
static uint8_t cmd_pending;

const char cli_prompt[] = ">> ";
const char cli_unrecognized[] = "[Error] Command not recognized.";

cli_status_t cli_process()
{
	int c = getchar_timeout_us(0);
	if (c != PICO_ERROR_TIMEOUT)
	{
		cli_put(c);
	}
	
	if (!cmd_pending)
		return CLI_IDLE;
	
	uint8_t *p = cmd_buffer;
	while (*p)
	{
		*p = tolower(*p);
		p++;
	}
	
	cmd_pending = 0;
	uint8_t argc = 0;
	char *argv[30];

	//puts((char*)cmd_buffer);
	/* Get the first token (cmd name) */
	argv[argc] = strtok((char*) cmd_buffer, " \r");
	cli_status_t ret = CLI_OK;
	/* Walk through the other tokens (parameters) */
	while ((argv[argc] != NULL) && (argc < 30))
	{
		argv[++argc] = strtok(NULL, " \r");
	}

	if (argc == 0)
	{
		/* nothing to do */
	}
	else if (strcmp("help", argv[0]) == 0)
	{
		puts("");
		puts("######################");
		puts("## LIST OF COMMANDS ##");
		puts("######################");
		for (size_t i = 0; i < cli.cmd_cnt; i++)
		{
			printf(cli.cmd_tbl[i].cmd);
			printf("\t: ");
			puts((char*) cli.cmd_tbl[i].help); /* Print the CLI help */
		}
		puts("");
	}
	else
	{
		ret = CLI_E_CMD_NOT_FOUND;
		/* Search the command table for a matching command, using argv[0]
		 * which is the command name. */
		for (size_t i = 0; i < cli.cmd_cnt; i++)
		{
			if (cli.cmd_tbl[i].cmd == NULL)
				break;
			if (strcmp(argv[0], cli.cmd_tbl[i].cmd) == 0)
			{
				/* Found a match, execute the associated function. */
				ret = cli.cmd_tbl[i].func(argc, argv);
				if (ret == CLI_E_INVALID_ARGS)
				{
					puts("[ERROR] Invalid Arguments. Usage:");
					puts(cli.cmd_tbl[i].help);
				}
			}
		}
	}

	if (ret == CLI_E_CMD_NOT_FOUND)
		puts((char*) cli_unrecognized);

	printf((char*) cli_prompt); /* Print the CLI prompt to the user.             */
	
	return ret;
}

void cli_init()
{
	buffer_ptr = buffer;

	cmd_pending = 0;

	/* Print the CLI prompt. */
	printf((char*) cli_prompt);
}

/* ANSI Terminal manipulation */
#define MOVE_TO_LINE_START "\r"
#define CLEAR_LINE "\33[2K\r"
#define MAX_ESCAPE_SEQUENCE 2
static bool s_bEsc = false;
static char s_EscSequence[MAX_ESCAPE_SEQUENCE] =
{};
static int s_EscCount = 0;
static int s_HistoryBack = 0;

static void CliEscHandling(void)
{
	if (strcmp(s_EscSequence, "[A") == 0)
	{
		if (s_HistoryIndex)
		{
			s_HistoryBack = (s_HistoryBack + 1) % MAX_HISTORY;
			int curr = (s_HistoryIndex - s_HistoryBack) % MAX_HISTORY;

			int len = strlen((char*) s_History[curr]);

			printf((char*)CLEAR_LINE);
			printf(cli_prompt);
			printf((char*)s_History[curr]);
			memcpy((char*) buffer, (char*) s_History[curr], len);
			buffer_ptr = buffer + len;
		}
	}
	if (strcmp(s_EscSequence, "[B") == 0)
	{
		s_HistoryBack = (s_HistoryBack - 1) % MAX_HISTORY;
		int curr = (s_HistoryIndex - s_HistoryBack) % MAX_HISTORY;

		int len = strlen((char*) s_History[curr]);

		printf((char*) CLEAR_LINE);
		printf(cli_prompt);
		printf((char*)s_History[curr]);
		memcpy((char*) buffer, (char*) s_History[curr], len);
		buffer_ptr = buffer + len;
	}
	if (strcmp(s_EscSequence, "[C") == 0)
	{
		//ConsoleTransmit((uint8_t *)"Right Arrow",12);
	}
	if (strcmp(s_EscSequence, "[D") == 0)
	{
		//ConsoleTransmit((uint8_t *)"Left Arrow",11);
	}
}

void cli_put(char c)
{
	if (s_bEsc)
	{
		s_EscSequence[s_EscCount++] = c;
		if (s_EscCount >= MAX_ESCAPE_SEQUENCE)
		{
			CliEscHandling();
			s_EscCount = 0;
			s_bEsc = false;
		}
		return;
	}
	
	switch (c)
	{
	case 022: /* CTRL-R */
		printf((char*) CLEAR_LINE);
		printf((char*) buffer);
		break;
	case 025: /* CTRL-U */
		buffer_ptr = buffer; /* Reset buf_ptr to beginning.                   */
		printf(CLEAR_LINE);
		break;
	case 033:
		s_bEsc = true;
		break;
	case '\b':
		/* Backspace. Delete character. */
		if (buffer_ptr > buffer)
		{
			buffer_ptr--;
			printf("\b \b");
		}
		break;
	case CMD_TERMINATOR:
		if (!cmd_pending)
		{
			*buffer_ptr = '\0'; /* Terminate the msg and reset the msg ptr.      */
			int curr = s_HistoryIndex % MAX_HISTORY;
			int len = strnlen((const char*) buffer, MAX_BUFFER_SIZE);
			if (strncmp((char*) s_History[curr], (char*) buffer, MAX_BUFFER_SIZE)) /* if same as the last command no need to update */
			{
				memcpy((char*) s_History[curr], (char*) buffer, len);
				s_History[curr][len] = 0; /* NULL terminate */
				if (buffer_ptr - buffer)
					s_HistoryIndex++; /* increment history index only if not an empty string */
			}
			memcpy((char*) cmd_buffer, (char*) buffer, len);
			cmd_buffer[len] = 0;
			cmd_pending = 1;
			buffer_ptr = buffer; /* Reset buf_ptr to beginning.                   */
		}
		putchar(c);
		putchar('\n');
		s_HistoryBack = 0;
		break;
		
	default:
		if ((buffer_ptr - buffer) < MAX_BUFFER_SIZE)
		{
			*buffer_ptr++ = c;
			*buffer_ptr = 0;
			putchar(c);
		}
	}
	
}