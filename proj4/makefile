# Project 4 Makefile
# Corey Johns
# COP4610 Spring 2013

CC = gcc47 -Wall -Wextra
CCO = $(CC) -o
CCC = $(CC) -c

clone.x: clone.o
	$(CCO) clone.x clone.o

clone.o: clone.c
	$(CCC) clone.c

clean:
	rm -f *.o *.x