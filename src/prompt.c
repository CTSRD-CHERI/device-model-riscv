#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"
#include "prompt.h"

static char line[1024];

int prompt_init(void) {

    /* If we don't have history.txt, need at least one existing history
    entry to set up the buffer correctly */
    linenoiseHistoryAdd("previously-entered");

    line[0] = '\0';

    printf("\r\nStarting prompt\r\n");
    return 0;
}


void linenoise_completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
        linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello there");
    }
    if (!strcasecmp(buf, "/q")) {
        linenoiseAddCompletion(lc,"/quit");
    }
    if (!strcasecmp(buf, "/c")) {
        linenoiseAddCompletion(lc,"/count");
    }
}


int prompt_poll(void) {
    int something_else=0;
    int ret = 0;
//    int c = uart_getchar_nonblock();
//    if (c!=-1)  printf("%c", c);
    ret = linenoiseEdit(line, sizeof(line), "\n\rhello>");
//    if (ret != -1) printf("%d\r\n",ret);
    /* ret is the number of characters returned */
    if (ret <= 0) {
        /* Nothing to do:
         *  0: empty command entered
         * -1: no command to return (still collecting characters)
         * -2: Ctrl-D was pressed so we'll quit (below)
         */
         something_else++;
    } else if (line[0] != '\0' && line[0] != '/') {
        printf("\r\necho: '%s'\r\n", line);
        linenoiseHistoryAdd(line); /* Add to the history. */
        linenoiseHistorySave("history.txt"); /* Save the history on disk. */
    } else if (!strncmp(line,"/historylen",11)) {
        /* The "/historylen" command will change the history len. */
        int len = atoi(line+11);
        linenoiseHistorySetMaxLen(len);
    } else if (!strncmp(line,"/count",6)) {
        /* Print the counter that's our background work to do */
        printf("\r\nCounter: %d\r\n", something_else);
    } else if (!strncmp(line,"/quit",5)) {
        printf("\r\nQuit command received. Exiting now.\r\n");
        ret = -2;
    } else if (line[0] == '/') {
        printf("\r\nUnreconized command: %s\r\n", line);
    }

    return 0;
}
