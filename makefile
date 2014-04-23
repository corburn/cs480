CFLAGS=-std=c99

run: partBResponse partBGuess
	@./partBGuess&
	@sleep 1
	@./partBResponse

partBResponse: partBResponse.o

partBGuess: partBGuess.o

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

.PHONY: clean run

clean:
	-rm -f *.o *.d partBResponse partBGuess guess response

include $(wildcard *.d)
