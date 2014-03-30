CFLAGS=-std=c99

all: db

db: db.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -MMD -MP -o $@ $< $(CFLAGS)

.PHONY: clean run

clean:
	-rm db *.o *.d test.db

run: all
	./db test.db

include $(wildcard *.d)
