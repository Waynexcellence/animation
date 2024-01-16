.PHONY: test

all:
	gcc -std=gnu99 anime.c -o anime -pthread
	gcc -std=gnu99 -g anime.c -o debug -pthread
test:
	gcc -std=gnu99 test.c -o test -pthread
clean:
	rm -f anime test