#ifndef _state_h_
#define _state_h_
#include <dirent.h>
#include "joblist.h"

// State struct for keeping directory, foreground, and running jobs state
typedef struct {
	jlist_t * jobs;
	char * dirn;
	char * hostname;
	char * username;
	int fg;
	char exit;
} state_t;

#endif
