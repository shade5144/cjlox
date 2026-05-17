CC := gcc
CFLAGS := -g -Og -Wall -Wextra -Wshadow -Wpointer-arith
INC := include
INC_FLAG := -I$(INC)
SRC_FILES := $(wildcard src/*)
OBJ_FILES := $(subst src/,, $(SRC_FILES:c=o))
ALL_PREREQ := $(patsubst %,build/%,$(OBJ_FILES))

main: $(OBJ_FILES) 
	$(CC) $(CFLAGS) $(ALL_PREREQ) main.c -o main $(INC_FLAG)

$(OBJ_FILES): %.o: src/%.c
	$(CC) $(CFLAGS) -c $^ -o build/$@ $(INC_FLAG)

clean:
	rm -f build/*
	rm -f main
	rm -f debug
