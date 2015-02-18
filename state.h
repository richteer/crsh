#ifndef _state_h_
#define _state_h_
#include <dirent.h>
#include "joblist.h"

typedef struct {
	jlist_t * jobs;
	char * dirn;
	char * hostname;
	char * username;
	int fg;
	char exit;
} state_t;

#endif
