#ifndef _task_h_
#define _task_h_

#define IN_NOTFOUND -1

typedef struct task_s {
	char * command;
	char ** argv;
	struct task_s * pipe;
	struct task_s * pipep;
	int active;
	int pid;
	int bg;
	int in;
	int out;
	int err;
} task_t;

int tk_init(void *, task_t *);
int tk_free(task_t *);
#endif
