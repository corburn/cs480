// _REENTRANT is deprecated and deliberately omitted
// Modern compilers do not include it. The program is instead compiled
// with the -pthread flag.
/**
 * db.c
 *
 * A collection of functions that use system calls to maintain a database
 * of Person structures with a name and ID.
 *
 * Compile and run the program passing it the name of a nonexistent file to see
 * a functional demonstration.
 *
 * @author Jason Travis
 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * struct Person - An entry in the database
 * @id: a unique number to identify the person
 * @name: the name of the person
 */
struct Person {
    int id;
    char name[50];
};

void addP(struct Person *p);
void closeDB(void);
void *demo(void *arg);
int getP(char *name);
void printDB(void);
void removeP(char *name);

int fd;
char *filename;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv) {
    pthread_t threads[5];
    filename = argv[1];

    if(argc < 2) {
        // Insufficient arguments
        fprintf(stderr, "usage: %s FILE", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open the database
    fd = open(argv[1],O_RDWR|O_CREAT,0644);
    if(fd == -1) {
        fprintf(stderr, "%s: Couldn't open file %s; %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Close the database on normal exit
    atexit(closeDB);

    // Launch threads
    for(int i=0; i<sizeof(threads)/sizeof(threads[0]); ++i) {
        if(pthread_create(&threads[i],NULL,demo,NULL) != 0) {
            perror("Thread creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to finish
    for(int i=0; i<sizeof(threads)/sizeof(threads[0]); ++i) {
        if(pthread_join(threads[i],NULL) != 0) {
            perror("Thread join failed");
            exit(EXIT_FAILURE);
        }
    }

    if(pthread_mutex_destroy(&mutex) != 0) {
        perror("Destroy mutex");
        exit(EXIT_FAILURE);
    }

    printf("Printing the database...\n");
    printDB();

    return EXIT_SUCCESS;
}

/**
 * addP - Append Person to database
 * @p: Pointer to the Person to add
 *
 * Prints the Person's ID and name when the user is successfully added
 */
void addP(struct Person *p) {
    pthread_mutex_lock(&mutex);
    printf("Adding %i:%s\n",p->id,p->name);
    // Append to the end of file
    if(lseek(fd,0,SEEK_END) == -1) {
        perror(NULL);
        return;
    }
    if(write(fd, p, sizeof(struct Person)) == -1) {
        perror(NULL);
        return;
    }
    pthread_mutex_unlock(&mutex);
}

/**
 * closeDB should be called by atexit() to automatically close the database on normal exit
 */
void closeDB(void) {
    close(fd);
}

/**
 * demo is executed by a thread demonstrating add and remove functionality
 */
void *demo(void *arg) {
    struct Person p;
    char buf[sizeof(p.name)];
    // Initialize Person name
    sprintf(buf,"Thread_%lu",pthread_self());
    strcpy(p.name,buf);
    printf("Thread %lu reporting for duty\n", pthread_self());
    // Add 10 entries where the name is the thread identifier
    // and the id is the loop iteration that created it
    for(int i=0; i<10; ++i) {
        p.id = i;
        addP(&p);
    }
    // Remove 9 entries
    for(int i=0; i<9; ++i) {
        removeP(buf);
    }
    pthread_exit(NULL);
}

/**
 * getP - Retrieve Person ID
 * @name: Name of the Person to find
 *
 * Return: ID of first match or -1 if none found
 */
int getP(char *name) {
    int id = -1;
    size_t nr;
    struct Person p;
    pthread_mutex_lock(&mutex);
    // Search from beginning of file
    if(lseek(fd,0,SEEK_SET) == -1) {
        perror(NULL);
    }
    // Find first occurrence of name
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        if(strcmp(name,p.name) == 0) {
            id = p.id;
            break;
        }
    }
    if(nr == -1) {
        perror(NULL);
    }
    printf("Found %d:%s\n",p.id,p.name);
    pthread_mutex_unlock(&mutex);
    return id;
}

/**
 * printDB - Print the name and ID of everyone in the database
 */
void printDB() {
    size_t nr;
    struct Person p;
    // Search from beginning of file
    if(lseek(fd,0,SEEK_SET) == -1) {
        perror(NULL);
    }
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        printf("%d:%s\n", p.id, p.name);
    }
    if(nr == -1) {
        perror(NULL);
    }
}

/**
 * removeP - Remove Person from database
 * @name: Name of the Person to remove
 *
 * Remove first occurence of Person with the given name from the database
 */
void removeP(char *name) {
    struct Person p;
    size_t nr;
    int count = 0;
    pthread_mutex_lock(&mutex);
    // Search from beginning of file
    if(lseek(fd,0,SEEK_SET) == -1) {
        perror("removeP lseek to beginning of file");
        return;
    }
    // Find first occurrence of name
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        ++count;
        if(strcmp(name,p.name) == 0) {
            // Print id and name as confirmation to the user the Person was removed
            printf("Removing %i:%s\n",p.id,p.name);
            break;
        }
    }
    if(nr == -1) {
        perror("removeP read to matching entry");
    }
    if(nr == 0) {
        // Reached EOF and found no match
        printf("%s not found\n",name);
        return;
    }
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        // Remove entry and close the gap by shifting all entries over one
        ++count;
        if(lseek(fd,-2*sizeof(p),SEEK_CUR) == -1) {
            perror("removeP lseek previous entry");
            return;
        }
        if(write(fd,&p,sizeof(p)) == -1) {
            perror("removeP overwrite previous entry");
            return;
        }
        if(lseek(fd,sizeof(p),SEEK_CUR) == -1) {
            perror("removeP lseek next entry");
            return;
        }
    }
    if(nr == -1) {
        perror("removeP read following entries");
    }
    // Truncate database to the number of entries minus the one removed
    truncate(filename,(count-1)*sizeof(p));
    if(pthread_mutex_unlock(&mutex) != 0) {
        perror("mutex unlock");
        exit(EXIT_FAILURE);
    }
}

