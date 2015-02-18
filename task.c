#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "task.h"
#include "joblist.h"
#include "state.h"
#include "error.h"

// Value outside the range of a legel return code
#define ISPARENT 0xFFF

static int tk_run(state_t * st, task_t * tk)
{
	int pid;
	int pipes[2] = {-1,-1};

	// Uhhh... this probably won't work...
	if (tk->pipe != NULL) {
		pipe(pipes);
		tk->out = pipes[1];
		tk->pipe->in  = pipes[0];
	}
/*
	printf("Running task: '%s", tk->command);
	char ** t;
	for (t = tk->argv; *t != NULL; t++)
		printf(" %s",*t);
	printf("' FDs: %d %d %d",tk->out, tk->in, tk->err);
	printf(" Pipe: %p\n", tk->pipe);
*/
	tk->active = 1;
	pid = fork();
	if (pid) { 
		tk->pid = pid;
		if (tk->pipe != NULL) 
			tk_run(st, tk->pipe);
		return ISPARENT;
	}

	if (tk->out != -1) {
		//dup2(STDOUT_FILENO, tk->out);
		dup2(tk->out, STDOUT_FILENO);
	}
	if (tk->in != -1) {
		//dup2(STDIN_FILENO, tk->in);
		dup2(tk->in, STDIN_FILENO);
	}
	if (tk->err != -1) {
		dup2(tk->err, STDERR_FILENO);
	}
	if (tk->pipe != NULL) close(tk->pipe->in);
	if (tk->pipep != NULL) close(tk->pipep->out);

	if (-1 == execvp(tk->command, tk->argv)) {
		perror("Failed to exec");
	}
	exit(1);
	return 0;
}


// TODO: Redo the args on this.
int tk_init(void * moo, task_t * tk)
{
	// Dirty hack to fix a stupid header problem
	state_t * st = (state_t *) moo;
	jnode_t * job;
	
	job = jl_new_node();
	job->jid = jl_next_jid(st->jobs);
	job->task = tk;
	jl_app_node(st->jobs, job);
	if (!tk->bg) {
		st->fg = job->jid;
	}	

	tk_run(st, tk);
	return 0; // TODO: Change this
}

// Frees one task, does not handle pipe-chained tasks
int tk_free(task_t * tk)
{
	char ** c;

	close(tk->out);
	close(tk->in);
	close(tk->err);
	free(tk->command);
	for (c = tk->argv; *c != NULL; c++) free(*c);
	free(tk->argv);	
	free(tk);

	return 0;
}

