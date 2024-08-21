COMPILER := clang
COMPILER_FLAGS := -std=c99 -Wextra -Wpedantic
SRCS := test.c
OBJS=$(subst .c,.o,$(SRCS))
BIN = test

$(BIN): $(OBJS)
	$(COMPILER) $(COMPILER_FLAGS) $^ -o $@

$(OBJS): %.o: %.c
	$(COMPILER) $(COMPILER_FLAGS) -c $^

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJS)