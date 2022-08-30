CASES := $(wildcard test_*.c)
CASES_OBJ := $(CASES:.c=.o)

.PHONY: run

run: test

test: test.o $(CASES_OBJ) test.h
	@echo Case sources: $(CASES)
	@echo Objects: $(CASES_OBJ)
	$(CC) -o $@ $^

test%.o: test%.c
	$(CC) -I../src -c -o $@ $<

clean:
	rm *.o test
