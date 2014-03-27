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
    char name[20];
};

void addP(const struct Person *p);
int getP(const char *name);
void removeP(const char *name);
void printDB(void);
void closeDB(void);
void lockDB(struct flock *region);
void unlockDB(const struct flock *region);
void demo(void);
int find(const char *name, struct flock *region);
void print_region(const char *prefix, const struct flock *region);
int count_entries(void);

static int fd;
static char *filename;

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
    //demo();
    struct Person p = {
        .id = 42,
        .name = "Alec"
    };
    addP(&p);
    addP(&p);

    // Launch 3 processes to modify the database
    // TODO reduce to 1 during debugging
    for(int i=0; i<1; ++i) {
        if(getpid() == parent) {
            if(fork() == -1) {
                perror(NULL);
            }
        }
    }

    // Split parent / child process execution paths
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
    static struct flock region = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_END,
        .l_start = 0,
        .l_len = sizeof(struct Person)
    };
    lockDB(&region);
    printf("Process_%d adding %i:%s\n",getpid(),p->id,p->name);
    // Append to the end of file
    if(lseek(fd,0,SEEK_END) == -1) {
        perror(NULL);
        return;
    }
    if(write(fd, p, sizeof(struct Person)) == -1) {
        perror(NULL);
        return;
    }
    unlockDB(&region);
}

/**
 * getP - Retrieve Person ID
 * @name: Name of the Person to find
 *
 * Return: ID of first match or -1 if none found
 */
int getP(const char *name) {
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
    return id;
}

/**
 * removeP - Remove Person from database
 * @name: Name of the Person to remove
 *
 * Remove first occurence of Person with the given name from the database
 */
void removeP(const char *name) {
    int index;
    int count;
    struct Person p;
    struct flock start = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = sizeof(p)
    };
    struct flock end = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_END,
        .l_start = sizeof(p),
        .l_len = sizeof(p)
    };
    printf("Process_%d searching for %s\n", getpid(), name);
    if((index = find(name,&start)) == -1) {
        // Reached EOF and no match found
        printf("Process_%d could not find %s\n",getpid(),name);
        return;
    }
    // Found a match, prepare to remove it
    lockDB(&end);
    // Upgrade lock on entry to a write lock
    // This should come after the lock on the last element to avoid potential deadlock
    start.l_type = F_WRLCK;
    lockDB(&start);
    printf("Process_%d removed %d:%s\n", getpid(), p.id, p.name);
    // Read the last entry
    if(lseek(fd,-sizeof(p),SEEK_END) == -1) {
        perror("removeP seek to last entry");
        return;
    }
    if(read(fd,&p,sizeof(p)) == -1) {
        perror("removeP read last entry");
        return;
    }
    // Overwrite the entry to be removed with the last entry
    if(lseek(fd,index*sizeof(p),SEEK_SET) == -1) {
        perror("removeP seek to matching entry");
        return;
    }
    if(write(fd,&p,sizeof(p)) == -1) {
        perror("removeP write over matching entry");
        return;
    }
    count = count_entries();
    // Truncate database to the number of entries minus the one removed
    if(truncate(filename,(count-1)*sizeof(p)) == -1) {
        perror("removeP truncate database");
    }
    unlockDB(&start);
    unlockDB(&end);
}

/**
 * printDB - Print the name and ID of everyone in the database
 */
void printDB(void) {
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

int find(const char *name, struct flock *region) {
    // Initialize to an invalid number
    int nr = -2;
    struct Person p;
    // Read from region->l_start to EOF or error
    while(nr != 0 && nr != -1) {
        // Wait for a lock on the region to be read
        lockDB(region);
        /*
         *if(fcntl(fd,F_SETLKW,region) == -1) {
         *    perror(NULL);
         *    break;
         *}
         */
        // Move to the locked region
        lseek(fd,region->l_start,SEEK_SET);
        // Read the region
        if((nr = read(fd,&p,sizeof(p))) == -1) {
            perror(NULL);
            break;
        }
        if(strcmp(name,p.name) == 0) {
            printf("Process_%d found %s\n", getpid(), name);
            // Return index of matching region
            return region->l_start / sizeof(p);
        }
        // Increment to the next region
        region->l_start += sizeof(p);
        /*
         *if(fcntl(fd,F_UNLCK,region) == -1) {
         *    perror(NULL);
         *    break;
         *}
         */
        unlockDB(region);
    }
    return -1;
}

/**
 * Should be called by atexit when the database is opened to automatically close
 * on normal exit.
 */
void closeDB(void) {
    close(fd);
}

/*
 * Prints prefix and the lock structure for debugging
 */
void print_region(const char *prefix, const struct flock *region) {
    char *types[] = {"F_RDLCK", "F_WRLCK", "F_UNLCK" } ;
    char *whence[] = {"SEEK_SET", "SEEK_CUR", "SEEK_END"} ;
    printf("%s:\n\tl_type: %s\n\tl_whence: %s\n\tl_start: %lu\n\tl_len: %lu\n\tl_pid: %d\n",prefix,types[region->l_type],whence[region->l_whence],region->l_start/sizeof(struct Person),region->l_len/sizeof(struct Person),region->l_pid);
}

/**
 * TODO: comments
 * lockDB - Lock the database
 * Wait until a lockfile is successfully created
 *
 * See unlockDB
 */
void lockDB(struct flock *region) {
    char prefix[80];
    struct flock buf = *region;
    do {
        while(region->l_type != F_UNLCK) {
            *region = buf;
            sprintf(prefix,"Process_%d F_GETLK",getpid());
            print_region(prefix,region);
            if(fcntl(fd,F_GETLK,region) == -1) {
                perror(NULL);
            }
            switch(region->l_type) {
                case F_RDLCK:
                    printf("Process_%d Process_%d has a read lock at index %lu\n", getpid(), region->l_pid, region->l_start/sizeof(struct Person));
                    break;
                case F_WRLCK:
                    printf("Process_%d Process_%d has a write lock at index %lu\n", getpid(), region->l_pid, region->l_start/sizeof(struct Person));
                    break;
            }
        }
        *region = buf;
        sprintf(prefix,"Process_%d F_SETLK", getpid());
        print_region(prefix,region);
    } while(fcntl(fd,F_SETLK,region) == -1);
}

/**
 * unlockDB - Unlock the database
 * Close and delete the lockfile
 *
 * See lockDB
 */
void unlockDB(const struct flock *region) {
    char prefix[80];
    sprintf(prefix,"Process_%d F_UNLCK", getpid());
    print_region(prefix,region);
    if(fcntl(fd,F_UNLCK,region) == -1) {
        perror(NULL);
    }
}

/**
 * countEntries - Number of entries in the database
 * Uses stat to calculate the number of database entries
 * Return: number of entries in the database
 */
int count_entries(void) {
    struct stat st;
    // Calculate number of entries based on the size of the file
    stat(filename,&st);
    return st.st_size/(sizeof(struct Person));
}
