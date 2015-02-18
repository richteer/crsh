CC=clang
CFLAGS=-g

all: crsh

crsh: crsh.o joblist.o error.o parse.o task.o internal.o
	${CC} $^ -o $@ -lreadline

crsh.o: crsh.c state.h
	${CC} -c ${CFLAGS} $<

joblist.o: joblist.c joblist.h
	${CC} -c ${CFLAGS} $<

error.o: error.c error.h
	${CC} -c ${CFLAGS} $<

parse.o: parse.c parse.h
	${CC} -c ${CFLAGS} $<

task.o: task.c task.h
	${CC} -c ${CFLAGS} $<

internal.o: internal.c internal.h
	${CC} -c ${CFLAGS} $<

clean:
	rm -rf crsh *.o
