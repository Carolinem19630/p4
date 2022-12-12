CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := server.c

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile.net libmfs.so
	${CC} $< -o $@ udp.c

clean:
	rm -f ${PROGS} *.o *.so


%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<

libmfs.so: libmfs.o
	gcc -shared -Wl,-soname,libmfs.so -o libmfs.so libmfs.o -lc	

libmfs.o: libmfs.c
	gcc -c -fPIC -g -Wall -Werror libmfs.c udp.c
