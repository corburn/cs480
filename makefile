CFLAGS=-std=c99
LDFLAGS=-pthread

run: db
	./db test.db

db: db.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

.PHONY: clean run

clean:
	-rm db -f *.o *.d test.db

include $(wildcard *.d)
