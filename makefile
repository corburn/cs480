CFLAGS=-std=c99

run: db
	./db test.db

db: db.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) -c -MMD -MP -o $@ $< $(CFLAGS)

.PHONY: clean run

clean:
	-rm db *.o *.d test.db

include $(wildcard *.d)
