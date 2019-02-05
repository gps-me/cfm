CC=gcc
CCFLAGS=-Wall -Wextra
LDFLAGS=-lncurses
EXEC=cfm
SRC=$(wildcard *.c)

cfm: cfm.c config.h utils.h
	$(CC) $(CCFLAGS) $(SRC) $(LDFLAGS) -o $(EXEC)

clean: 
	rm -rf $(EXEC)