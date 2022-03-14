#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device-model.h"
#include "prompt.h"

static char line[1024];

// this array is searched on every keystroke, so keep it short
struct keycode keydb[] = {
    {KEY_BS,	"\x7f"    },
    {KEY_DEL,	"\x1b[3~" },
    {KEY_UP,	"\x1b[A"  },
    {KEY_DOWN,  "\x1b[B"  },
    {KEY_LEFT,  "\x1b[D"  },
    {KEY_RIGHT, "\x1b[C"  },
    {KEY_HOME,  "\x1b[H"  },
    {KEY_END,   "\x1b[F"  },
    {KEY_INSERT,"\x1b[2~" },
    {-1,        ""        }
};


enum command_state {
	INIT, PROMPT, WAITING, BUSY
};


static int
prompt_readint(struct epw_softc *sc, char *command)
{
	printf("prompt_readint %s\n", command);
	return 0;
}

static int
prompt_writeint(struct epw_softc *sc, char *command)
{
	printf("prompt_writeint %s\n", command);
	return 0;
}

static int
prompt_command(struct epw_softc *sc, char *command)
{
	printf("Got %s\n", command);
	if (!strncmp(command, "ri ", 3)) {
		return prompt_readint(sc,command+3);
	} else if (!strncmp(command, "wi ", 3)) {
		return prompt_writeint(sc, command+3);
	}
	printf("Unrecognised command <%s>\n", command);
	return -1;
}

int
prompt_init(struct epw_softc *sc)
{
	return 0;
}

static int
prompt_handlekey(char *cmdline, size_t cmdline_length, int pos, int keytag)
{
	// we have detected a keycode for a control key
	switch (keytag) {
		case KEY_BS:
		case KEY_DEL:
			if (pos != 0) {
				cmdline[pos-1] = '\0';
				pos--;
				printf("\b \b");
			}
			break;
	}
	
	return pos;
}

static int
prompt_lookbehind(char *cmdline, size_t cmdline_length, int pos)
{
	int keyidx = 0;
	int keytag = 0;
	char *keychars = "";

	// sanity check we are never off the end of the buffer
	if (pos >= cmdline_length) {
		return pos;
	}
	// search the keycode array looking for matches
	while (keytag != -1) {
		keytag = keydb[keyidx].key;
		keychars = keydb[keyidx].codes;
		if (keytag != -1) {
			int keychars_len = strlen(keychars);
			// fail if too close to the line start
			if (pos < keychars_len) {
				break;
			}
			if (!strcmp((cmdline + pos) - keychars_len, keychars)) {
				//printf("Matched key %d\n", keytag);
				// wipe the keycode from the command string
				pos -= keychars_len;
				cmdline[pos] = '\0';
				// process the keycode
				pos = prompt_handlekey(cmdline, cmdline_length, pos, keytag);
				break;
			}
		}
		keyidx ++;
	}

	return pos;
		
}

int
prompt_poll(struct epw_softc *sc)
{
	static enum command_state state = INIT;
	char cmdline[1024];
	static int p = 0;
	int c;

	switch (state) {
		case INIT:
			p = 0;
			cmdline[0] = '\0';
			state = PROMPT;
			break;
		case PROMPT:
			printf("\n> ");
			state = WAITING;
			break;
		case WAITING:
			c = uart_getchar_nonblock();
			if (c != EOF) {
			    if (p+1 < sizeof(cmdline)/sizeof(char)) {
			        cmdline[p] = c;
			        cmdline[p+1] = '\0';
			        p++;
			        printf("%c", c);
//			        if (c<32 || c==127) printf("\\x%x",c); else printf("%c",c);
			    }
			    if (c == '\r' || c == '\n') {
			        cmdline[p-1] = '\0';
			        prompt_command(sc, cmdline);
			        cmdline[0]='\0';
			        p = 0;
			        state = PROMPT;
			    }
			    p = prompt_lookbehind(cmdline, sizeof(cmdline), p);
			}
			break;
		case BUSY:
			break;
	}

	return (0);
}


