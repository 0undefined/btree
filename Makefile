CC     = gcc
LFLAGS =
FLAGS  = -ansi -Wall -Wextra -pedantic
OUT    = btree-test
LIB_OUT    = libbtree.so
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

lib: DEFS += -fpic
lib: $(LIB_OUT)

$(LIB_OUT): $(OBJ)
	$(CC) $(DEFS) $(FLAGS) $(LFLAGS) -shared -o $(LIB_OUT) $(OBJ)

$(OUT): $(OBJ)
	$(CC) $(DEFS) $(FLAGS) $(LFLAGS) -o $(OUT) $(OBJ)

obj/%.o: src/%.c src/btree.h obj
	$(CC) $(DEFS) $(FLAGS) -c -o $@ $<

obj:
	mkdir -p $@

clean:
	rm -rf $(OUT) obj
