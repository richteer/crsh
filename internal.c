#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "internal.h"
#include "parse.h"

int inc_cd(state_t *, task_t *);
int inc_fg(state_t *, task_t *);
int inc_rp(state_t *, task_t *);
int inc_term(state_t *, task_t *);
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
	{ "rp", inc_rp },
	{ "term", inc_term },
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

int inc_rp(state_t * st, task_t * tk)
{
	int event, i, n, count;
	char * c;
	FILE * histfile;
	char * buffer;
	char * line;
	int bufsz;
	int mod;
	int termnum;

	if (tk->argv[1] == NULL) {
		printf("Need event to repeat\n");
		return 1;
	}

	event = atoi(tk->argv[1]);

	histfile = fopen("/tmp/.crsh_history","r");

	fseek(histfile, 0, SEEK_END);
	bufsz = ftell(histfile);
	rewind(histfile);

	buffer = malloc(bufsz);
	fread(buffer, 1, bufsz, histfile);

	fclose(histfile);

	for (i = 0; i < bufsz; i++) {
		if (buffer[i] == '\n') buffer[i] = '\0';
	}

	if (event < 0) {
		count = -2; // So that `rp -1` is not self-referential
		event = abs(event);
		for (c = buffer + bufsz - 1; c >= buffer; c--) {
			if (*c == '\0') {
				count++;
				c--;
			}
			if (count == event) {
				c += 2;
				goto done;
			}
		}
		printf("Could not find that event\n");
		free(buffer);
		return 4;
		
	}
	else if (event > 0) {
		count = 1;
		for (c = buffer; c < (buffer + bufsz); c++) {
			if (*c == '\0') {
				count++;
				c++;
			}
			if (count == event) {
				goto done;
			}
		}
		printf("Could not find that event\n");
		free(buffer);
		return 2;
	}
	else {
		printf("History is counted, not indexed\n");
		free(buffer);
		return 3;
	}

done:	
	printf("%s\n",c);
	parse(st, c);

	free(buffer);
	return 0;
}

int inc_term(state_t * st, task_t * tk)
{
	jnode_t * nd;
	task_t * t;

	if (tk->argv[1] == NULL) {
		printf("Need job id to terminate\n");
		return 1;
	}

	nd = jl_find_jid(st->jobs, atoi(tk->argv[1]));
	if (nd == NULL) {
		printf("Invalid job id\n");
		return 2;
	}

//	jl_rem_node(st->jobs, nd);

	for (t = nd->task; t != NULL; t = t->pipe) {
		kill(t->pid, SIGKILL);
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
