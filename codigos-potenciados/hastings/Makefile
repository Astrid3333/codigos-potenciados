CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra
LDLIBS = -lm

all: hastings_test

hastings_test: hastings.c hastings_test.c
	$(CC) $(CFLAGS) -o $@ hastings.c hastings_test.c $(LDLIBS)

clean:
	rm -f hastings_test *.o

.PHONY: all clean