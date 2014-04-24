CFLAGS=-std=c99

usage:
	@echo Run the command \'make A\' or \'make B\'
	@echo \'make A\' - compile and execute Part A
	@echo \'make B\' - compile and execute Part B

A: partA
	@./$^

B: partBResponse partBGuess
	@./partBGuess&
	@sleep 1
	@./partBResponse

partA: partA.c

partBResponse: partBResponse.c

partBGuess: partBGuess.c

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

.PHONY: A B clean

clean:
	-rm -f *.o *.d partA partBResponse partBGuess guess response

include $(wildcard *.d)
