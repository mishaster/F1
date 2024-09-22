obj-m += game_of_life.o

build:
	make -C /lib/modules/$(shell uname -r)/build modules M=$(PWD)

clean:
	make -C /lib/modules/$(shell uname -r)/build clean M=$(PWD)
	rm test main advanced

load: build
	sudo insmod game_of_life.ko

unload:
	sudo rmmod game_of_life

main: main.c
	gcc main.c -o main

test: test.c
	gcc test.c -Wall -Wextra -Wpedantic -o test

advanced: advanced.c
	gcc advanced.c -o advanced
