#ifndef DB_H
#define DB_H
/**
 * struct Person - An entry in the database
 * @id: a unique number to identify the person
 * @name: the name of the person
 */
struct Person {
    int id;
    char name[20];
};

void openDB(char *filename);
void addP(struct Person *p);
int getP(char *name);
void removeP(char *name);
void printDB();
int lockDB();
void unlockDB(int fdlock);

int fd;
char *filename;
#endif

