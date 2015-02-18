#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "joblist.h"
#include "error.h"

#define XOR(a,b) ((!(a) && (b)) || ((a) && !(b)))
#define BOTH(a,b) ((a->head==b)&&(a->tail==b))

static int jl_verify(jlist_t * ls)
{	
	if (ls == NULL) {
		error("Verify: List is NULL\n");
		return -1;
	}
	else if (XOR(ls->head == NULL,ls->tail == NULL)) {
		error("Verify: List head/tail mismatch\n");
		return -3;
	}
	return 0;
}

// TODO: Add rest of job stuff to args
jnode_t * jl_new_node(void)
{
	jnode_t * ret = malloc(sizeof(jnode_t));
	ret->task = NULL;
	ret->next = NULL;
	ret->prev = NULL;
	ret->jid = -1;

	return ret;
}

int jl_next_jid(jlist_t * ls)
{
	if (ls == NULL) return -999;
	if (ls->tail == NULL) return 0;
	return ls->tail->jid + 1;
}

int jl_app_node(jlist_t * ls, jnode_t * nd)
{
	int ret;

	if ((ret = jl_verify(ls))) {
		error("Verify failed in append\n");
		goto end;
	}
	/* Ensure node is not already part of a list */
	else if ((nd->next != nd->next) && (nd->prev == NULL)) {
		error("Append Node: Node contains non-null next/prev\n");
	}

	/* Empty list */
	if (BOTH(ls,NULL)) {
		ls->head = ls->tail = nd;
		ret = 0;
		goto end;
	}

	nd->prev = ls->tail;
	ls->tail->next = nd->prev;
	ls->head->next = nd;
	ls->tail = nd;
	
end:
	//jl_print(ls);
	return ret;
}

int jl_rem_node(jlist_t * ls, jnode_t * nd)
{
	int ret;
	jnode_t * cur;

	if ((ret = jl_verify(ls))) {
		error("Verify failed in remove\n");
		return ret;
	}
	
	if (nd == NULL) {
		error("Node is NULL\n");
		return 1;
	}
	else if (BOTH(ls,NULL)) {
		error("List is empty\n");
		return 2;
	}

	/* Only item in list */
	if (BOTH(ls,nd)) {
		ls->head = ls->tail = NULL;
		jl_free_nd(nd);
		ret = 0;
	}
	/* First item */
	else if(ls->head == nd) {
		ls->head = nd->next;
		nd->next->prev = NULL;
		jl_free_nd(nd);
		ret = 0;
	}
	/* Last item */
	else if (ls->tail == nd) {
		ls->tail = nd->prev;
		nd->prev->next = NULL;
		jl_free_nd(nd);
		ret = 0;
	}
	else {
		for (cur = ls->head; cur->next != NULL; cur = cur->next) {
			if (cur == nd) {
				cur->prev->next = cur->next;
				cur->next->prev = cur->prev;
				jl_free_nd(nd);
				goto end;
			}
		}
		error("Could not locate node in list!\n");
		return 3;
	}

end:

	return ret;
}

// TODO: Error below here
jnode_t * jl_find_jid(jlist_t * ls, int jid)
{
	jnode_t * cur;

	if ((ls->head == NULL) && (ls->tail == NULL)) {
		return NULL;
	}

	/* Check tail first, to avoid a whole traversal */
	if (ls->tail->jid == jid) return ls->tail;
	
	for (cur = ls->head; cur->next != NULL; cur = cur->next) {
		if (cur->jid == jid) return cur;
	}

	error("Could not find jid in list\n");
	return NULL;
}

jnode_t * jl_find_pid(jlist_t * ls, int pid)
{
	
	jnode_t * cur;
	task_t * tk;

	if ((ls->tail == NULL) && (ls->head == NULL)) return NULL;

	//if (ls->tail->task->pid == pid) return ls->tail;

	for (cur = ls->head; cur != NULL; cur = cur->next) {
		for (tk = cur->task; tk != NULL; tk = tk->pipe) {
			if (tk->pid == pid) return cur;
		}
	}
	
	error("Could not find pid in list\n");
	return NULL;
}

int jl_free_ls(jlist_t * ls)
{
	jnode_t *cur, *prev;

	for (cur = ls->head; cur != NULL;) {
		prev = cur;
		cur = cur->next;
		jl_free_nd(prev);
	}
	
	ls->head = NULL;
	ls->tail = NULL;
	return 0;
}

int jl_free_nd(jnode_t * nd)
{
	task_t * tk, *tmp;

	if (nd == NULL) {
		error("Tried to free a NULL node\n");
		return 1;
	}

	if (nd->task)
		tk_freeall(nd->task);
	// TODO: Kill process?
	free(nd);
	
	return 0;
}

int jl_print(jlist_t * ls) {
	int ret;
	jnode_t * cur;

	if ((ret = jl_verify(ls))) {
		error("Verify failed in print\n");
		goto end;
	}

	printf("Jobs:\n");

	for (cur = ls->head; cur != NULL; cur = cur->next) {
		printf("%c[%d] %s\n",' ',cur->jid, cur->task->command); // TODO: Make the printing a little more useful
	}

end:	
	return ret;
}

// Returns 1 if all of the nodes are no longer active
int jl_inactive_nd(jnode_t * nd)
{
	int ret = 0;
	task_t * tk;

	for (tk = nd->task; tk != NULL; tk = tk->pipe) {
		ret |= tk->active;
	}

	return !ret;
}

int jl_set_inactive_pid(jnode_t *nd, int pid)
{
	task_t * tk;

	for (tk = nd->task; tk != NULL; tk = tk->pipe) {
		if (tk->pid == pid) {
			tk->active = 0;
			close(tk->out);
			close(tk->in);
			close(tk->err);
			return 0;
		}		
	}

	return -1;
}

int jl_clear_pid(jlist_t * jl, jnode_t * nd, int pid)
{
	int ret = 0;	

	task_t * tk = nd->task;
	task_t * tmp = NULL;

	while (tk->pid != pid) {
		tmp = tk;
		tk = tk->pipe;
	}
	
	if (tmp != NULL) {
		tmp->pipe = tk->pipe;
	}
	else {
		nd->task = tk->pipe;
	}

	//printf("%s, %d %d %d\n",tk->command,close(tk->out),close(tk->in),close(tk->err));
	tk_free(tk);

	if (nd->task == NULL) {
		jl_rem_node(jl, nd);
		ret = 1;
	}
	else {
		//kill(nd->task->pid, SIGPIPE);
	}
	return ret;
}

