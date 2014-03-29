#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "./db.h"

int main(int argc, char **argv) {
    // Use the original pid to guarantee it is the only one forking processes
    pid_t parent = getpid();

    if(argc < 2) {
        // Insufficient arguments
        fprintf(stderr, "usage: %s FILE", argv[0]);
        exit(EXIT_FAILURE);
    }

    openDB(argv[1]);

    // Launch 3 processes to modify the database
    for(int i=0; i<3; ++i) {
        if(getpid() == parent) {
            fork();
        }
    }

    if(getpid() == parent) {
        // Wait for all child processes to return
        while(wait(NULL)) {
            if(errno == ECHILD) {
                break;
            }
        }
        // As the last step, print and close the database
        printDB();
        close(fd);
    } else {
        // Child processes run functional demonstration
        execl("./child", "child", filename);
        perror("execl");
    }
    return EXIT_SUCCESS;
}
