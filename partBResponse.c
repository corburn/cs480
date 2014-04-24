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
    int answer;

    atexit(closePipes);

    do {
        printf("Enter a number between 1 and %d: ", MAX);
        if(scanf("%d", &answer) == -1) {
            perror("Prompt for number");
        }
    } while(answer < 1 || answer > MAX);

    fdGuess = open(query, O_RDONLY);
    fdAnswer = open(response, O_WRONLY);
    startAnswering(answer);

    return 0;
}

void closePipes(void) {
    close(fdGuess);
    close(fdAnswer);
}

void startAnswering(int answer) {
    int nr, response, guess;
    printf("parent waiting for guess\n");
    while((nr = read(fdGuess, &guess, sizeof(guess))) != -1 && nr != 0) {
        printf("parent read %d\n",guess);
        if(guess < answer) {
            printf("Yes\n");
            response = 1;
        } else {
            printf("No\n");
            response = 0;
        }
        printf("parent write %d\n",response);
        write(fdAnswer,&response,sizeof(response));
    }

    if(nr == -1) {
        perror("read guess");
        exit(EXIT_FAILURE);
    }
}

