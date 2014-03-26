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
#include <sys/stat.h>
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

void addP(const struct Person *p);
int getP(const char *name);
void removeP(const char *name);
void printDB(void);
void closeDB(void);
int lockDB(void);
void unlockDB(const int fdlock);
void demo(void);
int countEntries(void);

static int fd;
static char *filename;
static struct flock region1 = {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(struct Person)
};
static struct flock region2 = {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(struct Person)
};

int main(int argc, char **argv) {
    // Use the original pid to guarantee it is the only one forking processes
    pid_t parent = getpid();

    if(argc < 2) {
        // Insufficient arguments
        fprintf(stderr, "usage: %s FILE", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Open the database
    filename = argv[1];
    // Automatically close the database on normal exit
    atexit(closeDB);
    if((fd = open(filename,O_RDWR|O_CREAT,0644)) == -1) {
        fprintf(stderr, "%s: Couldn't open file %s; %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // TODO create a database for debugging functions
    demo();

    // Launch 3 processes to modify the database
    for(int i=0; i<3; ++i) {
        if(getpid() == parent) {
            if(fork() == -1) {
                perror(NULL);
            }
        }
    }

    if(getpid() == parent) {
        // Wait for all child processes to return
        while(wait(NULL)) {
            if(errno == ECHILD) {
                break;
            }
        }
        printDB();
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
void addP(const struct Person *p) {
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
int getP(const char *name) {
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
void removeP(const char *name) {
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
void printDB(void) {
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
    char buf[20];
    sprintf(buf,"Process_%d",getpid());
    struct Person p;
    //char *names[] = {"Teofila", "Treva", "Dennis", "Hannah", "Inocencia", "Basil", "Melba", "Maricela", "Jeffery", "Alec"};
    // Add each name to the database
    for(int i = 0; i < 10; i++) {
        p.id = i;
        strcpy(p.name,buf);
        addP(&p);
    }
    // TODO uncomment when finished debugging locking
    // Remove all but one of the names from the database
    for(int i = 0; i < 9; i++) {
        removeP(buf);
    }
}

/**
 * lockDB - Lock the database
 * Wait until a lockfile is successfully created
 *
 * See unlockDB
 */
int lockDB(void) {
    int lockfile;
    while((lockfile = open("db.lock", O_CREAT|O_EXCL,0444)) == -1 && errno == EEXIST);
    if(lockfile == -1) {
        perror("lockDB");
    }
    return lockfile;
}

/**
 * unlockDB - Unlock the database
 * Close and delete the lockfile
 *
 * See lockDB
 */
void unlockDB(const int lockfile) {
    if(close(lockfile) == -1) {
        perror("unlockDB close");
    }
    if(unlink("db.lock") == -1) {
        perror("unlockDB unlink");
    }
}

