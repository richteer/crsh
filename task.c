#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "task.h"
#include "joblist.h"
#include "state.h"
#include "error.h"

// Value outside the range of a legel return code
#define ISPARENT 0xFFF

// Actually execute a task
static int tk_run(state_t * st, task_t * tk)
{
	int pid;
	int pipes[2] = {-1,-1};

	// Create pipes if needed
	if (tk->pipe != NULL) {
		pipe(pipes);
		tk->out = pipes[1];
		tk->pipe->in  = pipes[0];
	}

	tk->active = 1;
	pid = fork();

	if (pid) { 
		tk->pid = pid;

		// Recuse if there is a piped task to be ran
		if (tk->pipe != NULL) 
			tk_run(st, tk->pipe);
		return ISPARENT;
	}

	// Dup the pipes if they are set
	if (tk->out != -1) {
		dup2(tk->out, STDOUT_FILENO);
	}
	if (tk->in != -1) {
		dup2(tk->in, STDIN_FILENO);
	}
	if (tk->err != -1) {
		dup2(tk->err, STDERR_FILENO);
	}
	// Close the unused ends if there is a pipe involved
	if (tk->pipe != NULL) close(tk->pipe->in);
	if (tk->pipep != NULL) close(tk->pipep->out);

	if (-1 == execvp(tk->command, tk->argv)) {
		perror("Failed to exec");
	}
	exit(1);
	return 0;
}

// Entry point to create a job and start it
int tk_init(void * sta, task_t * tk)
{
	// Dirty hack to fix a stupid header problem
	state_t * st = (state_t *) sta;
	jnode_t * job;
	
	job = jl_new_node();
	job->jid = jl_next_jid(st->jobs);
	job->task = tk;
	jl_app_node(st->jobs, job);
	if (!tk->bg) {
		st->fg = job->jid;
	}

	tk_run(st, tk);
	return 0; 
}

// Frees one task, does not handle pipe-chained tasks
int tk_free(task_t * tk)
{
	char ** c;

	tk_setinactive(tk);
	
	//kill(tk->pid, SIGKILL);
	free(tk->command);
	for (c = tk->argv; *c != NULL; c++) free(*c);
	free(tk->argv);	
	free(tk);

	return 0;
}

// Free a task, and its chained piped tasks
int tk_freeall(task_t * tk)
{
	task_t * t, *tmp;
	for (t = tmp = tk; t != NULL;) {
		tmp = t;
		t = tmp->pipe;
		tk_free(tmp);
	}

	return 0;
}

// Close the file descriptors and set the task inactive (called during SIGCHLD)
int tk_setinactive(task_t * tk)
{
	tk->active = 0;
	if (-1 != tk->out) close(tk->out);
	if (-1 != tk->in)  close(tk->in);
	if (-1 != tk->err) close(tk->err);
	return 0;
}
