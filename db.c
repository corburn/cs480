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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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
int getP(char *name);
void removeP(char *name);
void printDB();
int lockDB();
void unlockDB(int fdlock);
void demo();

int fd;
char *filename;

int main(int argc, char **argv) {
    if(argc < 2) {
        // Insufficient arguments
        fprintf(stderr, "usage: %s FILE", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open Database
    filename = argv[1];
    fd = open(filename,O_RDWR|O_CREAT,0644);
    if(fd == -1) {
        fprintf(stderr, "%s: Couldn't open file %s; %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Only the original parent should fork processes
    pid_t parent = getpid();

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
        demo();
    }
    return EXIT_SUCCESS;
}

/**
 * addP - Append Person to database
 * @p: Pointer to the Person to add
 *
 * Prints the Person's ID and name when the user is successfully added
 */
void addP(struct Person *p) {
    int lock = lockDB();
    // Append to the end of file
    if(lseek(fd,0,SEEK_END) == -1) {
        perror(NULL);
        return;
    }
    if(write(fd, p, sizeof(struct Person)) == -1) {
        perror(NULL);
        return;
    }
    printf("Process_%d added %i:%s\n",getpid(),p->id,p->name);
    unlockDB(lock);
}

/**
 * getP - Retrieve Person ID
 * @name: Name of the Person to find
 *
 * Return: ID of first match or -1 if none found
 */
int getP(char *name) {
    int lock = lockDB();
    int id = -1;
    size_t nr;
    struct Person p;
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
    // Print ID of the person found
    printf("Process_%d found %d\n",getpid(),id);
    unlockDB(lock);
    return id;
}

/**
 * removeP - Remove Person from database
 * @name: Name of the Person to remove
 *
 * Remove first occurence of Person with the given name from the database
 */
void removeP(char *name) {
    int lock = lockDB();
    int count = 0;
    size_t nr;
    // Search from beginning of file
    if(lseek(fd,0,SEEK_SET) == -1) {
        perror(NULL);
        return;
    }
    // Find first occurence of name
    struct Person p;
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        ++count;
        if(strcmp(name,p.name) == 0) {
            // Print id and name as confirmation to the user the Person was removed
            printf("Process_%d removed %i:%s\n",getpid(),p.id,p.name);
            break;
        }
    }
    if(nr == -1) {
        perror(NULL);
    }
    if(nr == 0) {
        // Reached EOF and found no match
        printf("Process_%d could not find %s\n",getpid(),name);
        return;
    }
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        // Remove entry and close the gap by shifting all entries over one
        ++count;
        if(lseek(fd,-2*sizeof(p),SEEK_CUR) == -1) {
            perror(NULL);
            return;
        }
        if(write(fd,&p,sizeof(p)) == -1) {
            perror(NULL);
            return;
        }
        if(lseek(fd,sizeof(p),SEEK_CUR) == -1) {
            perror(NULL);
            return;
        }
    }
    if(nr == -1) {
        perror(NULL);
    }
    // Truncate database to the number of entries minus the one removed
    truncate(filename,(count-1)*sizeof(p));
    unlockDB(lock);
}

/**
 * printDB - Print the name and ID of everyone in the database
 */
void printDB() {
    int lock = lockDB();
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
    unlockDB(lock);
}

/**
 * demo - Demonstrate program functionality
 *
 * Add entries to an example database, remove, and find an entry.
 */
void demo() {
    struct Person p;
    char *names[] = {"Teofila", "Treva", "Dennis", "Hannah", "Inocencia", "Basil", "Melba", "Maricela", "Jeffery", "Alec"};
    // Add each name to the database
    for(int i = 0; i < sizeof(names)/sizeof(char*); i++) {
        p.id = i;
        strcpy(p.name,names[i]);
        addP(&p);
    }
    // Remove all but one of the names from the database
    for(int i = 0; i < sizeof(names)/sizeof(char*)-1; i++) {
        removeP(names[i]);
    }
}

/**
 * lockDB - Create a lockfile
 *
 * Wait until the lockfile is successfully created
 * see unlockDB
 */
int lockDB() {
    int lockfile;
    while((lockfile = open("db.lock", O_CREAT|O_EXCL,0444)) == -1 && errno == EEXIST) {
        //perror("open");
    }
    return lockfile;
}

/**
 * unlockDB - Release the lockfile
 *
 * Close and delete the lockfile
 * see lockDB
 */
void unlockDB(int lockfile) {
    close(lockfile);
    //perror("close");
    unlink("db.lock");
    //perror("unlink");
}

