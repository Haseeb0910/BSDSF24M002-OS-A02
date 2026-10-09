CC      = gcc
CFLAGS  = -Wall -Wextra -g
SRC     = src/ls-v1.0.0.c
OBJ     = obj/ls-v1.0.0.o
BIN     = bin/ls

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o $(BIN)

.PHONY: all clean
