CC     = gcc
LFLAGS =
FLAGS  = -ansi -Wall -Wextra -pedantic
OUT    = btree-test
SRC   :=$(wildcard src/*.c)
OBJ   :=$(addprefix obj/,$(notdir $(SRC:.c=.o)))

.PHONY: clean

test: debug
	./$(OUT)

gdb: debug src/btree.h
	gdb -q -ex r $(OUT)

debug: DEFS += -g3 -DDEBUG
debug: obj build

build: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(DEFS) $(FLAGS) $(LFLAGS) -o $(OUT) $(OBJ)

obj/%.o: src/%.c src/btree.h
	$(CC) $(DEFS) $(FLAGS) -c -o $@ $<

obj:
	mkdir -p $@

clean:
	rm -rf $(OUT) obj
