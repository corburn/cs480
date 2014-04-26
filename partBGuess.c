// The maximum number the user can enter
// It must be a power of 2
#define MAX 1024

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void closePipes(void);
void startAnswering(int answer);
void startGuessing(int max);

static int fdGuess, fdAnswer;
static char *query = "query", *response = "response";


int main(int argc, char **argv) {
    printf("Guess start\n");

    // Close pipes on successful exit
    atexit(closePipes);

    if(mkfifo(query, 0777) == -1) {
        perror("mkfifo query");
        exit(EXIT_FAILURE);
    }

    if(mkfifo(response, 0777) == -1) {
        perror("mkfifo response");
        exit(EXIT_FAILURE);
    }

    if((fdGuess = open(query, O_WRONLY)) == -1) {
        perror("open query pipe O_WRONLY");
    }
    if((fdAnswer = open(response, O_RDONLY)) == -1) {
        perror("open response pipe O_RDONLY");
    }
    // Send guesses through the query pipe
    startGuessing(MAX);
    printf("Guess done\n");
    exit(EXIT_SUCCESS);

    return 0;
}

// Called by atexit to close and delete the pipes on successful exit
void closePipes(void) {
    if(close(fdGuess) == -1) {
        perror("close query");
    }
    if(close(fdAnswer) == -1) {
        perror("close query");
    }
    if(unlink(query) == -1) {
        perror("unlink query");
    }
    if(unlink(response) == -1) {
        perror("unlink response");
    }
}

// Send guesses through the query pipe and listen for answers
// on the response pipe
void startGuessing(int max) {
    int i = 2;
    int response;
    int guess = max/2;
    printf("child initialize guess %d\n", guess);
    while(max >> i > 0) {
        printf("Is the answer bigger than %d?\n",guess);
        printf("child write %d\n",guess);
        write(fdGuess,&guess,sizeof(guess));
        read(fdAnswer,&response,sizeof(response));
        printf("child read %d\n",response);
        if(response) {
            guess += max >> i;
        } else {
            guess -= max >> i;
        }
        ++i;
    }
     /*Fix off-by-one for even numbers by checking one more time*/
    printf("Is the answer bigger than %d?\n",guess);
    write(fdGuess,&guess,sizeof(guess));
    read(fdAnswer,&response,sizeof(response));
    if(response) {
        guess += 1;
    }
    printf("The answer is %d\n", guess);
}
