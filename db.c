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
#include <sched.h>
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
int lockDB(struct flock *region);
int lockDBW(struct flock *region);
int unlockDB(struct flock *region);
void demo(void);
int find(const char *name, struct flock *region);
void print_region(const char *prefix, const struct flock *region);
int count_entries(void);

static int fd;
static char *filename;

int main(int argc, char **argv) {
    // Use the original pid to guarantee it is the only one forking processes
    pid_t parent = getpid();
    printf("Process_%d parent process\n",getpid());

    if(argc < 2) {
        // Insufficient arguments
        fprintf(stderr, "usage: %s FILE", argv[0]);
        return EXIT_FAILURE;
    }

    // Open the database
    filename = argv[1];
    // Automatically close the database on normal exit
    atexit(closeDB);
    if((fd = open(filename,O_RDWR|O_CREAT,0644)) == -1) {
        fprintf(stderr, "%s: Couldn't open file %s; %s\n", argv[0], argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    // Launch 3 processes to modify the database
    for(int i=0; i<3; ++i) {
        if(getpid() == parent) {
            if(fork() == -1) {
                perror("main fork");
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
        printf("Process_%d reporting for duty\n", getpid());
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
    struct flock region = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_END,
        .l_start = 0,
        .l_len = sizeof(struct Person)
    };

    // Wait for EOF lock
    if(lockDBW(&region) == -1) {
        return;
    }
    // Append Person to EOF
    printf("Process_%d adding %i:%s\n",getpid(),p->id,p->name);
    if(lseek(fd,region.l_start,region.l_whence) == -1) {
        perror("addP lseek");
        return;
    }
    if(write(fd, p, sizeof(struct Person)) == -1) {
        perror("addP write");
        return;
    }
    // Release lock
    unlockDB(&region);
}

/**
 * getP - Retrieve Person ID
 * @name: Name of the Person to find
 *
 * Return: ID of first match or -1 if none found
 */
int getP(const char *name) {
    struct Person p;
    struct flock region = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = sizeof(p)
    };

    // Find entry matching name
    if(find(name,&region) == -1) {
        printf("Process_%d could not find %s\n",getpid(),name);
        return -1;
    }
    // Read and print entry
    read(fd,&p,sizeof(p));
    printf("Process_%d found %d:%s\n",getpid(),p.id,p.name);
    // Release the lock created by find()
    unlockDB(&region);
    return p.id;
}

/**
 * removeP - Remove Person from database
 * @name: Name of the Person to remove
 *
 * Remove first occurence of Person with the given name from the database
 */
void removeP(const char *name) {
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
        .l_start = -sizeof(p),
        .l_len = sizeof(p)
    };
    while(1) {
        // Search database for an entry with the given name
        if(find(name,&start) == -1) {
            // Reached EOF and no match found
            printf("Process_%d could not find %s\n",getpid(),name);
            return;
        }
        // Found a match, prepare to remove it
        // Upgrade entry lock to a write lock
        start.l_type = F_WRLCK;
        if(lockDB(&start) == -2 || lockDB(&end) == -2) {
            // Potential deadlock
            // Release all locks and try again later
            unlockDB(&start);
            unlockDB(&end);
            sched_yield();
        } else {
            // Acquired locks
            break;
        }
    }
    // Read the last entry
    if(lseek(fd,end.l_start,end.l_whence) == -1) {
        perror("removeP seek to last entry");
        return;
    }
    if(read(fd,&p,sizeof(p)) == -1) {
        perror("removeP read last entry");
        return;
    }
    // Overwrite the entry to be removed with the last entry
    if(lseek(fd,start.l_start,start.l_whence) == -1) {
        perror("removeP seek to matching entry");
        return;
    }
    if(write(fd,&p,sizeof(p)) == -1) {
        perror("removeP write over matching entry");
        return;
    }
    // Truncate database to the number of entries minus the one removed
    count = count_entries();
    if(truncate(filename,(count-1)*sizeof(p)) == -1) {
        perror("removeP truncate database");
    }
    printf("Process_%d removed %d:%s\n", getpid(), p.id, p.name);
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
        perror("printDB lseek");
    }
    while((nr = read(fd,&p,sizeof(p))) > 0) {
        printf("%d:%s\n", p.id, p.name);
    }
    if(nr == -1) {
        perror("printDB read");
    }
}

/**
 * demo - Demonstrate program functionality
 *
 * Add entries to an example database, remove, and find an entry.
 */
void demo(void) {
    char buf[20];
    sprintf(buf,"Process_%d",getpid());
    struct Person p;
    //char *names[] = {"Teofila", "Treva", "Dennis", "Hannah", "Inocencia", "Basil", "Melba", "Maricela", "Jeffery", "Alec"};
    // Add each name to the database
    // TODO reduce during debugging
    for(int i = 0; i < 3; i++) {
        p.id = i;
        strcpy(p.name,buf);
        addP(&p);
    }
    // TODO uncomment when finished debugging locking
    // Remove all but one of the names from the database
    // TODO reduce during debugging
    for(int i = 0; i < 3; i++) {
        removeP(buf);
    }
    removeP(buf);
}

/**
 * find - find a matching entry
 * @name name of entry to find
 * @region lock to find and hold the entry
 *
 * Return index of matching entry
 *
 * Searches the database for an entry matching the given name
 * starting from the given region start position to EOF
 */
int find(const char *name, struct flock *region) {
    // Initialize to an invalid number
    int nr = -2;
    struct Person p;
    printf("Process_%d searching for %s\n", getpid(), name);
    // Read from region->l_start to EOF or error
    while(nr != 0 && nr != -1) {
        // Wait for a lock on the region to be read
        if(lockDBW(region) == -1) {
            break;
        }
        // Move to the locked region
        lseek(fd,region->l_start,region->l_whence);
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
        if(unlockDB(region) == -1) {
            break;
        }
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
    printf("%s:\n\tl_type: %s\n\tl_whence: %s\n\tl_start: %ld\n\tl_len: %lu\n\tl_pid: %d\n",prefix,types[region->l_type],whence[region->l_whence],region->l_start,region->l_len,region->l_pid);
}

/**
 * lockDB - Lock the database
 *
 * Return -1 on error or -2 if a lock was not acquired within 3 attempts
 * See unlockDB
 */
int lockDB(struct flock *region) {
    int tries;
    int fcntl_status;
    char prefix[80];
    struct flock buf = *region;
    do {
        for(tries = 3; tries>0 && region->l_type != F_UNLCK; --tries) {
            *region = buf;
            sprintf(prefix,"Process_%d F_GETLK",getpid());
            print_region(prefix,region);
            if(fcntl(fd,F_GETLK,region) == -1) {
                perror(prefix);
                return -1;
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
        // Potential deadlock
        if(tries == 0) {
            return -2;
        }
        *region = buf;
        sprintf(prefix,"Process_%d F_SETLK", getpid());
        print_region(prefix,region);
    } while((fcntl_status = fcntl(fd,F_SETLK,region)) == -1);
    if(fcntl_status == -1) {
        perror(prefix);
        return -1;
    }
    return 0;
}

/**
 * lockDBW - Lock the database
 * Wait until a lock is successfully acquired
 *
 * Return -1 on error
 *
 * See unlockDB
 */
int lockDBW(struct flock *region) {
    char prefix[80];
    sprintf(prefix,"Process_%d F_SETLKW",getpid());
    print_region(prefix,region);
    if(fcntl(fd,F_SETLKW,region) == -1) {
        perror(prefix);
        return -1;
    }
    return 0;
}

/**
 * unlockDB - Unlock the database
 * Release the region lock
 *
 * See lockDB
 */
int unlockDB(struct flock *region) {
    char prefix[80];
    sprintf(prefix,"Process_%d F_UNLCK", getpid());
    region->l_type = F_UNLCK;
    print_region(prefix,region);
    if(fcntl(fd,F_SETLK,region) == -1) {
        perror(prefix);
        return -1;
    }
    return 0;
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
