TARGET = file_sys
CC = gcc
FLAGS = -O3 -std=c11 -lm
OBJ = fs.o main.o

.PHONY: clean

all: clean $(TARGET)

%.o: %.c
	$(CC) $(FLAGS) $< -c -o $@
$(TARGET): $(OBJ)
	$(CC) $(FLAGS) $^ -o $@
clean:
	rm -rf $(OBJ) $(TARGET)