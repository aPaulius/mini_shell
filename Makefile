# PA Mini Shell Makefile

CC = gcc
FLAGS = -Wall
OBJ = pa_mini_shell.c

all: $(OBJ)
	$(CC) $(FLAGS) -o pa_mini_shell $(OBJ)