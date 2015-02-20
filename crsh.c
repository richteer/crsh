#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include "state.h"
#include "error.h"
#include "parse.h"

// ANSI Color Escape Sequence
#define CL_RED     "\x1b[31m"
#define CL_GREEN   "\x1b[32m"
#define CL_YELLOW  "\x1b[33m"
#define CL_BLUE    "\x1b[34m"
#define CL_MAGENTA "\x1b[35m"
#define CL_CYAN    "\x1b[36m"
#define CL_RESET   "\x1b[0m"

// Array of prompt colors to cycle through
char color_cycle[][10] = {
	CL_RED,
	CL_YELLOW,
	CL_GREEN,
	CL_CYAN,
	CL_BLUE,
	CL_MAGENTA,
};

// A regretfully globally allocated state structure
state_t state = {};

// Returns an allocated string for the prompt
static char * get_prompt(state_t * foo)
{
	static int curcolor = 0;
	char * prompt;
	int offset;
	prompt = calloc(1,256);
	
	for (offset = strlen(state.dirn); offset != 0; offset--) {
		if (state.dirn[offset] == '/') {
			offset++;
			break;
		}
	}

	sprintf(prompt,"%s[%s@%s %s]$ %s", color_cycle[curcolor], state.username, state.hostname, state.dirn + offset, CL_RESET);
	// Rotate the colors
	curcolor++;
	curcolor = curcolor % ((sizeof(color_cycle)/sizeof(char*))-1);
	return prompt;
}

// Handles a child exiting NOT stopping
void handle_sigchld(int signum, siginfo_t * si, void * wat)
{
	jnode_t * nd = jl_find_pid(state.jobs, si->si_pid);
	if (nd == NULL) {
		printf("Could not find pid...?\n");
		return;
	}

	// Set the task as inactive
	jl_set_inactive_pid(nd, si->si_pid);
	if(jl_inactive_nd(nd)) {
		jl_rem_node(state.jobs, nd);
		state.fg = -1; // Remove from foreground
	}

	// Reap the process
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Handle the ^Z signal, redir to foregrounded child
void handle_sigtstp()
{
	jnode_t * nd;
	task_t * tk;

	if (state.fg == -1) {
		return;
	}

	// Remove from foreground
	nd = jl_find_jid(state.jobs, state.fg);
	if (nd == NULL) {
		printf("Could not find job id... something is wonky here\n");
		state.fg = -1;
		return;
	}

	// Relay signal (may not be needed)
	for (tk = nd->task; tk != NULL; tk = tk->pipe) {
		kill(tk->pid, SIGTSTP);
	}
	state.fg = -1;
}

// Passively ignore sigint
void handle_sigint()
{

}

int main(int argc, char ** argv)
{
	char * line = NULL;
	char * prompt = NULL;
	int jid;
	struct sigaction sa;
	struct passwd * user;

	set_errlog(stderr);

	/* Initialize Signals */
	sa.sa_sigaction = handle_sigchld;
	sa.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);
	signal(SIGTSTP, handle_sigtstp);
	signal(SIGINT, handle_sigint);


	/* Initialize State */
	state.fg = -1;

	state.jobs = calloc(1,sizeof(jlist_t));
	state.dirn = calloc(1, 256);
	getcwd(state.dirn, 255);
	
	/* Load the information for prompt. (only once, does not update on su or something */
	user = getpwuid(getuid());
	state.username = calloc(1,strlen(user->pw_name)+1);
	strcpy(state.username, user->pw_name);

	state.hostname = calloc(1,256);
	gethostname(state.hostname, 255);

begin:
	while (!state.exit) {
		prompt = get_prompt(&state);
		line = readline(prompt);
		
		// Checks for ^D or a failure to read a line
		if (line == NULL) {
			fputc('\n', stdout);
			free(line);
			free(prompt);
			break;
		}

		if (line[0] != '\0') {
			parse(&state, line);
		}
		while (state.fg >= 0) { 
			// Waiting waiting waiting...
		}
		free(line);
		free(prompt);
	}
	// Still active jobs...
	if (state.jobs->head != NULL) {
		state.exit = 0;
		printf("There are still active jobs.\n");
		goto begin;
	}

	// Cleanup
	free(state.dirn);
	free(state.hostname);
	free(state.username);
	jl_free_ls(state.jobs);
	free(state.jobs);

	return 0;
}
