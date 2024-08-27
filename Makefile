CC=gcc
LIB=-lreadline
OUTPUT=minishell

all: main.o
        $(CC) $(LIB) main.o -o  $(OUTPUT)
main.o:
        $(CC) -c main.c 
