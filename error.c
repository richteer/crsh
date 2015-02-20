#include <stdio.h>
#include <stdlib.h>
#include "error.h"

FILE * errlog = NULL;

int error(char * str)
{
	if (errlog == NULL) return -1;
	return fprintf(errlog, str);
}

void set_errlog(FILE * fp)
{
	errlog = fp;
}
