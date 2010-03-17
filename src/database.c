/* Copyright 1989-93 GROUPE BULL -- See license conditions in file COPYRIGHT */

/*
 * database.c
 */

/*
 * V 1.41
 */

#include "database.h"
#include <stdio.h>

int DB_DebugLevel = 0;

/****************************************************************************
 * Private part
 ****************************************************************************/

static void
db_Msg(char *s)
{
   printf(s);
}

static void
db_Msg1(char *s, int n)
{
   printf(s, n);
}

static void
db_Err(char *s)
{
   fprintf(stderr, s);
}

static int
db_FirstFree(DataBase db)
{
   int i = db -> currentPointer, done = 0;
   while (! done) {
     if (i >= db -> allocated) done = 1;
     else if (db -> activeEntries[i] != DB_OCCUPIED)
       done = 1;
     else i++;
   }

   db -> currentPointer = i + 1;
   if (i >= db -> allocated) {
      db_Err( "db_FirstFree: something wrong occured\n");
      return 0;
   }
   else
     return i;
}

static Bool
db_IsFull(DataBase db)
{
   return (Bool) (db -> allocated == db -> number);
}


static Bool
db_IsActive(DataBase db, int i)
{
   return (Bool) (db -> activeEntries[i] == DB_OCCUPIED);
}

Generic *
db_FindEntry(DataBase db, Generic gen)
{
   Generic *result;
   int i;

   for (i=0; i < db -> allocated; i++) {
      if (db_IsActive(db, i)) {
         if (memcmp(db -> entries[i], gen, db -> sizeEntries) == 0) {
            return & db -> entries[i];
         }
      }
   }

   return NULL;
}

/****************************************************************************
 * Public part
 ****************************************************************************/

DataBase
DB_NewDataBase(int size, void *freeFunction)
{
   DataBase result;

   NEW(result, struct _DataBase);

   result -> allocated = 0;
   result -> number = 0;
   result -> chunkSize = 50;
   result -> currentPointer = 0;
   result -> activeEntries = NULL;
   result -> sizeEntries = size;
   result -> freeFunction = freeFunction;
   result -> entries = NULL;

   return result;
}

/*---------------------------------------------------------------------------*/

Bool
DB_EndOfDataBase(DataBase db)
{
  int i = db -> currentPointer;
  Bool result;

/* Any entries in there? */
   if (db -> activeEntries == NULL || db -> entries == NULL) return True;

/* Find the first valid entry */
   while (i < db -> allocated && db -> activeEntries[i] != DB_OCCUPIED) {
      i++;
   }

/* Is there a valid entry? */
   if (i < db -> allocated) {
      if (DB_DebugLevel == 1) db_Msg("endofdatabase returning False\n");
      result = False;
   }

/*  No, we're at the end */
   else {
      if (DB_DebugLevel == 1) db_Msg("endofdatabase returning True\n");
      result = True;
   }

   return result;
}

/*---------------------------------------------------------------------------*/

Bool
DB_AddEntry(DataBase db, Generic entry)
{
   Bool result;
   int i, firstFree;
   Generic *newEntry;

   if (DB_DebugLevel == 1) db_Msg("adding entry\n");

/* Check if we need to allocate some more */
   if (db_IsFull(db)) {

   /* Yes, allocate a new list, and a new allocation table */
      Generic *newList = (Generic *) malloc(sizeof(*newList) *
                              (db -> allocated + db -> chunkSize));
      int *alloc = (int *) malloc(sizeof(*alloc) *
                              (db -> allocated + db -> chunkSize));

   /* Check that the allocation could be done */
      if (newList == NULL || alloc == NULL) {
   	   db_Err( "DB_AddEntry : couldn't alloc\n");
	      return 1;
      }

   /* Copy the old one into it */
      for (i=0; i < db -> number; i++) {
         newList[i] = db -> entries[i];
         alloc[i] = db -> activeEntries[i];
      }

   /* Initialize the new ones */
      for (; i < db -> allocated + db -> chunkSize; i++) {
	newList[i] = NULL;
	alloc[i] = NULL;
      }

   /* Free the old lists */
      free(db -> entries);
      free(db -> activeEntries);

   /* And replace the old one with the new one */
      db -> entries = newList;
      db -> activeEntries = alloc;

   /* Don't forget to update the allocated items number */
      db -> allocated += db -> chunkSize;

   /* And initialize the newcomers to 0 */
      for (; i < db -> allocated; i++)
         newList[i] = 0;
   }

/* Locate the first free slot */
   DB_Rewind(db);
   firstFree = db_FirstFree(db);

/* Now I can safely add the new entry */
   newEntry = (Generic *) malloc(db -> sizeEntries);
   memcpy(newEntry, entry, db -> sizeEntries);
   db -> entries[firstFree] = newEntry;
   db -> activeEntries[firstFree] = DB_OCCUPIED;
   db -> number++;

   return 0;
}

/*---------------------------------------------------------------------------*/

Bool
DB_RemoveEntry(DataBase db, Generic entry)
{
   int i = 0;

   if (DB_DebugLevel == 1) db_Msg("removing entry\n");
   for (i=0; i < db -> allocated; i++) {
      if (db_IsActive(db, i)) {
         if (memcmp(db -> entries[i], entry, db -> sizeEntries) == 0) {
            db -> number--;
            db -> activeEntries[i] = 0;
	    if (db -> freeFunction)
	      ((void (*)()) db -> freeFunction)(db -> entries[i]);
	    free(db -> entries[i]);
	    return 0;
         }
      }
   }
   db_Err( "DB_RemoveEntry : couldn't remove entry\n");
   return 1;
}

/*---------------------------------------------------------------------------*/

Bool
DB_RemoveNthEntry(DataBase db, int n)
{
   Generic gen;
   DB_Rewind(db);

   if (DB_DebugLevel == 1) db_Msg1("removing %dth entry\n", n);
   if (n >= db -> number) {
      db_Err( "DB_RemoveNthEntry : n is too big\n");
      return 0;
   }

   gen = DB_NextEntry(db);
   while (n--) gen = DB_NextEntry(db);
   return DB_RemoveEntry(db, gen);
}

/*---------------------------------------------------------------------------*/

Bool
DB_ReplaceEntry(DataBase db, Generic old, Generic new)
{
   Bool result;
   Generic *gen;

   if (DB_DebugLevel == 1) db_Msg("replacing entry\n");
   gen = db_FindEntry(db, old);
   if (! gen) {
      db_Err("DB_ReplaceEntry : couldn't find entry\n");
      return 1;
   }

   *gen = new;

   return 0;
}

/*---------------------------------------------------------------------------*/

void
DB_Rewind(DataBase db)
{
   db -> currentPointer = 0;
}

/*---------------------------------------------------------------------------*/

Generic
DB_NextEntry(DataBase db)
{
   int i = db -> currentPointer;
   Generic result;

/* Any entries in there? */
   if (db -> activeEntries == NULL || db -> entries == NULL) return NULL;

/* Find the first valid entry */
   while (i < db -> allocated && db -> activeEntries[i] != DB_OCCUPIED) {
      i++;
   }

/* Set pointer correctly for a subsequent call to DB_NextEntry() */
   db -> currentPointer = i + 1;

/* Is there a valid entry? */
   if (i >= db -> allocated) { /* no, we're at the end */
     if (DB_DebugLevel == 1) db_Msg("next entry reached end of database\n");
     result = NULL;
   }
   else {   /* yes! */
      if (DB_DebugLevel == 1) db_Msg("next entry returning a value\n");
      result = db -> entries[i];
   }

   return result;
}

/*---------------------------------------------------------------------------*/

Generic
DB_NthEntry(DataBase db, int n)
{
   Generic result;
   DB_Rewind(db);

   if (DB_DebugLevel == 1) db_Msg1("returning %dth entry\n", n);
   if (n >= db -> number) {
      db_Err( "DB_NthEntry : n is too big\n");
      return NULL;
   }

   result = DB_NextEntry(db);
   while (n--) result = DB_NextEntry(db);

   return result;
}

/*---------------------------------------------------------------------------*/

int
DB_Count(DataBase db)
{
   return db -> number;
}

/*---------------------------------------------------------------------------*/

void
DB_ClearDataBase(DataBase db)
{
   int i;
   for (i = 0; i < db -> number; i++)
      db -> activeEntries[i] = 0;

   db -> number = 0;
   db -> currentPointer = 0;
}

/*---------------------------------------------------------------------------*/

void
DB_DestroyDataBase(DataBase db)
{
   Generic entry;
   DB_Rewind(db);
   while (! DB_EndOfDataBase(db)) {
      entry = DB_NextEntry(db);
      DB_RemoveEntry(db, entry);
   }
   free(db -> activeEntries);
   free(db -> entries);
}

/*---------------------------------------------------------------------------*/

void
DB_Sort(DataBase db, int (* compareFunction)(Generic a, Generic b))
{
  Generic data;
  Generic datap;
  int i, count;

/* In order to be able to use qsort, I must gather all the entries in a */
/* contiguous area of memory. Then I call qsort, clear the database and */
/* add the sorted entries one by one */

/* Allocate a memory chunk large enough to contain the database... */
  count = db -> number;
  data = (Generic) malloc(count * db -> sizeEntries);
  datap = data;

/* ... copy the entries into it... */
  DB_Rewind(db);
  for (i = 0; i < count; i++) {
    memcpy(datap, DB_NextEntry(db), db -> sizeEntries);
    datap = & ((char *) datap) [db -> sizeEntries];   /* strange increment! */
  }

/* ... sort them... */
  qsort(data, count, db -> sizeEntries, compareFunction);

/* ... and put the sorted entries back in the database */
  DB_ClearDataBase(db);
  datap = data;
  for (i = 0; i < count; i++) {
    DB_AddEntry(db, datap);
    datap = & ((char *) datap) [db -> sizeEntries];
  }

  free(data);
}

/*---------------------------------------------------------------------------*/

void
DB_DisplayDataBase(DataBase db, void (* displayFunction)(Generic a))
{
  int oldPointer = db -> currentPointer;
  DB_Rewind(db);

  while (! DB_EndOfDataBase(db)) {
    displayFunction(DB_NextEntry(db));
  }
  db -> currentPointer = oldPointer;
}
