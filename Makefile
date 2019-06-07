help:
	@echo "targets debug small fast"

clean:
	rm -f elsmod

SRCDIR=src/

SRCS=$(wildcard $(SRCDIR)*.c)

CC=gcc

CFLAGS=-Wall -Wextra -Wfatal-errors

debug:	CFLAGS += -g -Og
fast:	CFLAGS += -O2
small:	CFLAGS += -Os -s -flto

debug:	objs
fast:	objs
small:	objs

objs:
	$(CC) $(CFLAGS) $(SRCS) -o elsmod
