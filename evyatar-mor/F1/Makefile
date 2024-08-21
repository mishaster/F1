CC = clang
AS = as
LD = ld
FLAGS = -std=c2x -Weverything -Wno-declaration-after-statement -Wno-c++98-compat -Wno-padded -Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion -Wno-pre-c2x-compat -D_GNU_SOURCE
OUT = test
BUILD_PATH=./


all: test

test: test.c
	$(CC) $(FLAGS) $? -o $(BUILD_PATH)$(OUT)

clean:
	rm -rf $(BUILD_PATH)$(OUT) $(BUILD_PATH)*.o