CC=gcc
LIB=-lreadline
OUTPUT=minishell
FLAGS= -Wall -Wextra -pedantic -Werror

all: main.o
	$(CC) $(LIB) main.o -o  $(OUTPUT)
main.o: main.c
	$(CC) $(FLAGS) -c main.c 
