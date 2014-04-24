// The maximum number the user can enter
// It must be a power of 2
#define MAX 1024

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

struct Pipe {
    int guess[2];
    int answer[2];
};

void closePipes(void);
void startAnswering(int answer);
void startGuessing(int max);

struct Pipe p;

int main(int argc, char **argv) {
    int answer;

    // Prompt until a valid number is entered
    do {
        printf("Enter a number between 1 and %d: ", MAX);
        if(scanf("%d", &answer) == -1) {
            perror("Prompt for number");
        }
    } while(answer < 1 || answer > MAX);

    if(pipe(p.guess) == -1) {
        perror("create guess pipe");
        exit(EXIT_FAILURE);
    }

    if(pipe(p.answer) == -1) {
        perror("create answer pipe");
        exit(EXIT_FAILURE);
    }

    if(atexit(closePipes) == -1) {
        perror("atexit close pipe");
        exit(EXIT_FAILURE);
    }

    switch(fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            // Close pipe ends that will not be used by the child
            close(p.guess[0]);
            close(p.answer[1]);
            // Send guesses
            startGuessing(MAX);
            // Finished with pipe, close remaining ends
            close(p.guess[1]);
            close(p.answer[0]);
            break;
        default:
            // Close pipe ends that will not be used by the parent
            close(p.guess[1]);
            close(p.answer[0]);
            // Respond to guesses
            startAnswering(answer);
            // Finished with pipe, close remaining ends
            close(p.guess[0]);
            close(p.answer[1]);
            break;
    }

    return 0;
}

void closePipes(void) {
    close(p.guess[0]);
    close(p.guess[1]);
    close(p.answer[0]);
    close(p.answer[1]);
}

void startAnswering(int answer) {
    int nr, response, guess;
    printf("parent waiting for guess\n");
    while((nr = read(p.guess[0], &guess, sizeof(guess))) != -1 && nr != 0) {
        printf("parent read %d\n",guess);
        if(guess < answer) {
            printf("Yes\n");
            response = 1;
        } else {
            printf("No\n");
            response = 0;
        }
        printf("parent write %d\n",response);
        write(p.answer[1],&response,sizeof(response));
    }

    if(nr == -1) {
        perror("read guess");
        exit(EXIT_FAILURE);
    }
}

void startGuessing(int max) {
    int i = 2;
    int response;
    int guess = max/2;
    printf("child initialize guess %d\n", guess);
    while(max >> i > 0) {
        printf("Is the answer bigger than %d?\n",guess);
        printf("child write %d\n",guess);
        write(p.guess[1],&guess,sizeof(guess));
        read(p.answer[0],&response,sizeof(response));
        printf("child read %d\n",response);
        if(response) {
            guess += max >> i;
        } else {
            guess -= max >> i;
        }
        ++i;
    }
    // Fix off-by-one for even numbers by checking one more time
    printf("Is the answer bigger than %d?\n",guess);
    write(p.guess[1],&guess,sizeof(guess));
    read(p.answer[0],&response,sizeof(response));
    if(response) {
        guess += 1;
    }
    printf("The answer is %d\n", guess);
}
