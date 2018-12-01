#!/bin/bash -x

CC=gcc
CFLAGS=-g 
T1 = oss
T2 = user
OBJS1 = oss.o 
OBJS2 = user.o
all: $(T1) $(T2)
$(T1): $(OBJS1)
	$(CC) $(CFLAGS) -o $(T1) $(OBJS1)
$(T2): $(OBJS2)
	$(CC) $(CFLAGS) -o $(T2) $(OBJS2)
ass1.o: oss.c
	$(CC) $(CFLAGS) -c oss.c
child.o: user.c
	$(CC) $(CFLAGS) -c user.c 
clean:
	rm -f $(T1) $(T2) *.o

