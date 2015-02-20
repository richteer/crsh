#ifndef _joblist_h_
#define _joblist_h_
#include "task.h"

/* Structs */
// Doubly linked list node for jobs
typedef struct jnode_s {
	struct jnode_s * next;
	struct jnode_s * prev;
	task_t * task;
	int jid;
} jnode_t;

// "Main" list object
typedef struct jlist_s {
	jnode_t * head;
	jnode_t * tail;	
} jlist_t;


jnode_t * jl_new_node();

int jl_next_jid(jlist_t *);

int jl_app_node(jlist_t *, jnode_t *);

int jl_rem_node(jlist_t *, jnode_t *);

jnode_t * jl_find_jid(jlist_t *, int jid);

jnode_t * jl_find_pid(jlist_t *, int pid);

int jl_free_ls(jlist_t *);

int jl_free_nd(jnode_t *);

int jl_print(jlist_t *);

int jl_set_inactive_pid(jnode_t *, int);

int jl_inactive_nd(jnode_t *);

// Deprecated
int jl_clear_pid(jlist_t *, jnode_t *, int);

#endif
