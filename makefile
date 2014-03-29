CFLAGS=-std=c99

all: main child

main: main.o db.o
	$(CC) -o $@ $^

child: child.o db.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -MMD -MP -o $@ $< $(CFLAGS)

.PHONY: clean run

clean:
	-rm main child *.o *.d test.db

run: all
	./main test.db

include $(wildcard *.d)
