#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "db.h"

int main(int argc, char ** argv) {
    struct Person p;
    char process_id[20];

    // Initialize p.name with a process identifier
    sprintf(process_id,"Process_%d",getpid());
    strncpy(p.name,process_id,sizeof(process_id));

    openDB(argv[1]);

    // Add entries
    for(int i=0; i<10; ++i) {
        p.id = i;
        addP(&p);
    }

    // Remove entries
    for(int i=0; i<9; ++i) {
        removeP(process_id);
    }

    _exit(0);
}

