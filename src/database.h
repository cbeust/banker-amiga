/*
 * database.h
 */

/*
 * V 1.41
 */

/* Generic package to handle databases. No memory limit : chunks are */
/* allocated dynamically whenever needed */


#ifndef DATABASE_H
#define DATABASE_H

/* General definitions */
#define NEW(v, t) v = (t *) malloc(sizeof(t))
#ifndef True
#define True 1
#define False 0
#endif

/* Magic cookie */
#define DB_OCCUPIED 9

/* The generic type */
#ifndef Generic
#define Generic void *
#endif

/* Boolean type */
#ifndef Bool
#define Bool char
#endif

/* The structure itself */
typedef struct _DataBase {
   int allocated;               /* number of entries allocated */
   int number;                 /* number of entries actually active */
   int chunkSize;             /* size of chunk to allocate when needed */
   int currentPointer;       /* number of entry to be examined next */
   int *activeEntries;      /* grid flagging all valid entries */
   int sizeEntries;        /* size (in bytes) of one entry */
   void *freeFunction;  /* function to free a generic entry */
   Generic *entries;     /* the entries themselves */
} *DataBase;

DataBase
DB_NewDataBase(int size, void *freeFunction);
/* Initialize a new database, which objects have size 'size' */
/* and which entries can be deallocated with freeFunction() */

Bool
DB_EndOfDataBase(DataBase db);
/* True if we reached the end of the database */

void
DB_ClearDataBase(DataBase db);
/* Clear the database */

void
DB_DestroyDataBase(DataBase db);
/* Destroy the database, freeing all its entries */

Bool
DB_EndOfDataBase(DataBase db);
/* True if we reached the end of the database */

Bool
DB_AddEntry(DataBase db, Generic entry);
/* Add the following entry into the database */
/* Alter the internal pointer */
/* Return 0 if the operation was successful */

Bool
DB_RemoveEntry(DataBase db, Generic entry);
/* Remove the specified entry */
/* Return 0 if the operation was successful */

Bool
DB_RemoveNthEntry(DataBase db, int n);
/* Remove the nth DB_NextEntry from the database (0 = first entry) */
/* Return 0 if the operation was successful */

Bool
DB_ReplaceEntry(DataBase db, Generic old, Generic new);
/* Replace the old entry with the new one */
/* Return 0 if the operation was successful */

void
DB_Rewind(DataBase db);
/* Rewind the database so that a subsequent NextEntry returns the */
/* first occupied element */
/* Alter the internal pointer */

Generic
DB_NextEntry(DataBase db);
/* Return the next entry, or NULL if we reached the end of the database */
/* Alter the internal pointer */

Generic
DB_NthEntry(DataBase db, int n);
/* Return the nth "DB_NextEntry" of the database (0 = first entry) */

int
DB_Count(DataBase db);
/* Return the number of entries in the databae */

void
DB_Sort(DataBase db, int (* compareFunction)(Generic a, Generic b));
/* Sort the database, so that */
/*     compareFunction(DB_NextEntry(db), DB_NextEntry(db)) < 0  */

void
DB_DisplayDataBase(DataBase db, void (* displayFunction)(Generic a));
/* Display the database by calling the displayFunction on each entry */
/* Internal pointer is not modified */
#endif


