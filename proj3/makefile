# Project 3 Makefile
# Corey Johns
# COP4610 Spring 2013


CC = gcc47 -Wall -Wextra -lpthread
CCO = $(CC) -o
CCC = $(CC) -c

all: matrix.x producer-consumer.x

matrix.x: matrix.o
	$(CCO) matrix.x matrix.o

matrix.o: matrix.c
	$(CCC) matrix.c

producer-consumer.x: producer-consumer.o
	$(CCO) producer-consumer.x producer-consumer.o

producer-consumer.o: producer-consumer.c buffer.h
	$(CCC) producer-consumer.c
	
clean:
	rm -f *.o *.x