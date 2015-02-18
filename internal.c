#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "internal.h"

int inc_cd(state_t *, task_t *);
int inc_fg(state_t *, task_t *);
int inc_jobs(state_t *, task_t *);
int inc_exit(state_t *, task_t *);
int inc_history(state_t *, task_t *);

struct in_cmd_s {
	char cmd[32];
	int (*func)(state_t*, task_t *);
};

struct in_cmd_s in_cmdlist[] = {
	{ "cd", inc_cd },
	{ "fg", inc_fg },
	{ "exit", inc_exit },
	{ "jobs", inc_jobs },
	{ "history", inc_history },
};

int in_numcmd = sizeof(in_cmdlist)/sizeof(struct in_cmd_s);


int in_tryint(state_t * st, task_t * tk)
{
	int i;
	
	for (i = 0; i < in_numcmd; i++) {
		if (!strcmp(tk->command, in_cmdlist[i].cmd)) {
			return in_cmdlist[i].func(st,tk);
		}
	}

	return IN_NOTFOUND;
}

/******* SEA OF INTERNAL COMMANDS ********/

int inc_cd(state_t * st, task_t * tk)
{
	
	if (tk->argv[1] == NULL) {
		printf("Needs directory to change to\n");
		return 1;
	}

	chdir(tk->argv[1]);
	st->dirn = calloc(1, 256);
	getcwd(st->dirn, 255);
	
	return 0;
}

int inc_fg(state_t * st, task_t * tk)
{
	jnode_t * nd;
	task_t * t;
	
	if (tk->argv == NULL) {
		return 1;
	}
	else if (tk->argv[1] == NULL) {
		return 1;
	}
	nd = jl_find_jid(st->jobs, atoi(tk->argv[1]));
	if (nd == NULL) {
		printf("Invalid job id!\n");
	return 2;
	}

	st->fg = nd->jid;

	for (t = tk; t != NULL; t = tk->pipe) {
		kill(t->pid, SIGCONT);
	}

	return 0;
}

int inc_jobs(state_t * st, task_t * tk)
{
	jl_print(st->jobs);

	return 0;
}

int inc_exit(state_t * st, task_t * tk)
{
	st->exit = 1;

	return 0;
}

int inc_history(state_t * st, task_t * tk)
{
	FILE * histfile;
	char * buffer;
	int bufsz;
	int i;
	int lnum = 0;
	char * c;

	histfile = fopen("/tmp/.crsh_history", "r");

	if (histfile == NULL) {
		printf("Could not open ~/.crsh_history\n");
		return 1;
	}

	fseek(histfile, 0, SEEK_END);
	bufsz = ftell(histfile);
	rewind(histfile);

	buffer = malloc(sizeof(char)*bufsz);
	fread(buffer, 1, bufsz, histfile); 
	fclose(histfile);

	c = buffer;
	for (i = 0; i < bufsz; i++) {
		if (buffer[i] == '\n') {
			buffer[i] = '\0';
			printf("%4d  %s\n", ++lnum, c);
			c = buffer + i + 1;
		}
	}

	free(buffer);

	return 0;
}
