TARGET = mem_manager
CC = gcc
FLAGS = -O3 -std=c11 -lm
OBJ = memory.o process_sim.o mem_manager.o

.PHONY: clean

all: clean $(TARGET)

%.o: %.c
	$(CC) $(FLAGS) $< -c -o $@
$(TARGET): $(OBJ)
	$(CC) $(FLAGS) $^ -o $@
clean:
	rm -rf $(OBJ) $(TARGET)