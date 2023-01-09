CC     = gcc
LFLAGS =
FLAGS  = -ansi -Wall -Wextra -pedantic
OUT    = btree-test
SHARED_OUT = libbtree.so
STATIC_OUT = libbtree.a
SRC   :=$(wildcard src/*.c)
OBJ   :=$(addprefix obj/,$(notdir $(SRC:.c=.o)))

.PHONY: clean check

test: debug
	./$(OUT)

gdb: debug src/btree.h
	gdb -q -ex r $(OUT)

debug: DEFS += -g3 -DDEBUG
debug: obj build

build: $(OUT)

static: FLAGS += -fpic
static: $(STATIC_OUT)

shared: FLAGS += -fpic
shared: $(SHARED_OUT)

$(SHARED_OUT): $(OBJ)
	$(CC) $(DEFS) $(FLAGS) $(LFLAGS) -shared -o $(SHARED_OUT) $(OBJ)

$(STATIC_OUT): $(OBJ)
	ar -rcs -o $(STATIC_OUT) $(OBJ)

$(OUT): $(OBJ)
	$(CC) $(DEFS) $(FLAGS) $(LFLAGS) -o $(OUT) $(OBJ)

obj/%.o: src/%.c src/btree.h obj
	$(CC) $(DEFS) $(FLAGS) -c -o $@ $<

obj:
	mkdir -p $@

check:
	cppcheck --enable=all --suppress=unusedFunction src

clean:
	rm -rf $(OUT) obj
