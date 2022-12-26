#variables
CC		= gcc -pthread -lpthread
EXEC	= lift_sim_A
CFLAGS 	= -Wall -ansi -pedantic -Werror -std=c99
OBJ 	= lift_sim.o file_reader.o

$(EXEC) : $(OBJ)
			$(CC) $(OBJ) -o $(EXEC)

lift_sim.o : lift_sim.c lift_sim.h file_reader.h
			$(CC) -c lift_sim.c $(CFLAGS)

file_reader.o : file_reader.c file_reader.h
			$(CC) -c file_reader.c $(CFLAGS)

clean:
			rm -f $(EXEC) $(OBJ)
