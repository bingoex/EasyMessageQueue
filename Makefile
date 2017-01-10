
ARGS = -g -Wall -fno-strict-aliasing
CC = g++

CSRC=$(shell ls *.c)
CPPSRC=$(shell ls *.cpp)
OBJS=channel.o 

CHANNEL_LIB = ./cm_channel.a -I./

COMM_DIR = ./comm/src
COMM_LIB = $(COMM_DIR)/cm_lib.a -I$(COMM_DIR) 

All:prepare target test

prepare:
	make -C comm

.c.o:
	$(CC) $(ARGS) -c $*.c $(COMM_LIB)

target:
	$(CC) $(ARGS) -c channel.c $(COMM_LIB)
	rm -f cm_channel.a
	ar -q cm_channel.a $(OBJS)

test:test.o
	$(CC) $(ARGS) -o test test.o $(CHANNEL_LIB) $(COMM_LIB)

clean:
	make clean -C comm
	rm -rf *.o *.a test
