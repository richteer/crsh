#ifndef _task_h_
#define _task_h_

#define IN_NOTFOUND -1

// Individual task, like `ls -l`. Piped command lines translate into a linked list of tasks
typedef struct task_s {
	char * command;
	char ** argv;
	struct task_s * pipe;  // Command after the current in the pipeline
	struct task_s * pipep; // Command before the current in the pipeline
	int active;            // If the task has not completed (i.e. exited)
	int pid;               // PID when the task in exec'd
	int bg;                // Boolean if the task is supposed to be backgrounded

	// File descriptors for I/O redirection
	int in;
	int out;
	int err;
} task_t;

int tk_init(void *, task_t *);
int tk_free(task_t *);
int tk_freeall(task_t *);
int tk_setinactive(task_t *);

#endif
