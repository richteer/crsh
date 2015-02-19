#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "parse.h"
#include "error.h"
#include "task.h"
#include "internal.h"

#define ST_ERROR    0x0
#define ST_START    0x1
#define ST_COMMAND  0x2
#define ST_ARG      0x3
#define ST_ARGSPACE 0x4
#define ST_REDIRECT 0x5

static task_t * gentask(char * line)
{
	int state = ST_START;
	int fd = -1;
	int oflags = 0;
	int * stream;
	task_t * tk;
	char * c, *d;
	char * cmd_start;
	char * arg_begin;
	char ** argv;
	int arglength = 4;
	int numargs = 0;
	char * redir;

	argv = calloc(1,sizeof(char*) * arglength);
	tk = calloc(1,sizeof(task_t));

	tk->out = -1;
	tk->in = -1;
	tk->err = -1;

	for (c = line; *c != '\0'; c++) {
		switch(state) {
			case ST_START:
				if  (*c == '!') {
					tk->command = calloc(1,3);
					strcpy(tk->command, "rp");
					for (d = ++c; ;d++) {
						if (isdigit(*d)) continue;
						if (isspace(*d)) { *d = '\0'; break; }
						if (*d == '\0') break;
						printf("Error?\n");
					}
					argv[1] = calloc(1, strlen(c));
					strcpy(argv[1], c);
					numargs = 1;
					goto end;
				}
				else if (*c != ' ') {
					state = ST_COMMAND;
					cmd_start = c;
				}
				break;
			case ST_COMMAND:
				switch(*c) {
					case ' ':
						*c = '\0';
						//c++; // Advance off the NULL to prevent the for terminating early.
						state = ST_ARGSPACE;
						tk->command = calloc(1,strlen(cmd_start)+1);
						strcpy(tk->command, cmd_start);
						break;
					default: break; // Skip ASCII characters
				}
				break;
			case ST_ARG:
				if (*c != ' ') break;
				state = ST_ARGSPACE;
				numargs++;				
				if (numargs >= arglength) {
					arglength <<= 1;
					if (realloc(argv, arglength*sizeof(arglength)) == NULL)
						error("Wat\n");
				}
				*c = '\0';
				//c++;
				argv[numargs] = calloc(1,strlen(arg_begin)+1);
				strcpy(argv[numargs],arg_begin);
				break;
			case ST_ARGSPACE:
				if (*c == ' ') break;
				else if (*c == '|') {
					tk->pipe = gentask(c+1);
					tk->pipe->bg = 1;
					tk->pipe->pipep = tk;
					goto end;
				}
				else if (*c == '&') {
					tk->bg = 1;
					goto end;
				}
				else if ((*c == '>') || (*c == '<') || (*c == '#')) {
					redir = c;
					state = ST_REDIRECT;
				}
				else {
					arg_begin = c;
					state = ST_ARG;
				}
				break;
			case ST_REDIRECT:
				if ((*c == ' ') || (*(c+1) == '\0')) {
					if (*c == ' ') *c = '\0'; // Clean this
					switch(*redir) {
						case '<': oflags = O_RDONLY; stream = &tk->in; break;
						case '>': oflags = O_WRONLY | O_CREAT; stream = &tk->out; break;
						case '#': oflags = O_WRONLY | O_CREAT; stream = &tk->err; break;
					}

					fd = open(redir+1, oflags, 0644);
					if (fd == -1) {
						printf("Could not open file '%s'\n", redir+1);
						free(argv); // FIXME: Leaks any arguments
						free(tk->command);
						free(tk);
						return NULL;
					}
					*stream = fd;
					state = ST_ARGSPACE;
				}
				break;
			default:
				fprintf(stderr,"Something went wrong in parsing, in state %d\n",state);
		}
	}
	// Triggers if there are no args.
	if (state == ST_COMMAND) {
		tk->command = calloc(1,strlen(cmd_start)+1);
		strcpy(tk->command, cmd_start);
	}
	else if (state == ST_ARG) {
		numargs++;				
		if (numargs >= arglength - 1) {
			arglength <<= 1;
			if (realloc(argv, arglength*sizeof(arglength)) == NULL)
				error("Wat\n");
		}
		*c = '\0';
		c++;
		argv[numargs] = calloc(1,strlen(arg_begin)+1);
		strcpy(argv[numargs],arg_begin);
	}

end:
	tk->argv = argv;
	tk->argv[0] = calloc(1,strlen(tk->command)+1);
	strcpy(tk->argv[0], tk->command);
	tk->argv[numargs+1] = NULL;

	return tk;
}

static int writetohistory(task_t * tk)
{
	char ** c;
	FILE * histfile = fopen("/tmp/.crsh_history","a");

	if (histfile == NULL) {
		return -1;
	}
	fprintf(histfile, "%s", tk->command);
	for (c = tk->argv+1; *c != NULL; c++) {
		fprintf(histfile, " %s", *c);
	} 
	fprintf(histfile, "\n");

	fclose(histfile);

	return 0;
}

int parse(state_t * st, char * line)
{
	int ret;
	task_t * tk = gentask(line);
	if (tk == NULL) {
		return -1;
	}
	writetohistory(tk);
	ret = in_tryint(st, tk);
	if (ret == IN_NOTFOUND) {
		ret = tk_init(st, tk);
	}
	else if (ret != 0) {
		// Handle errors from internal
	}
	return ret;
}

