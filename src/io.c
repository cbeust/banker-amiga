/*
 * io.c
 */

#include <libraries/iffparse.h>

#include "common.h"


/*******************************************************************
 * Database file
 *******************************************************************/
 
static void
readDatabaseFile(Entries entries, struct IFFHandle *iff)
{
   struct ContextNode *cn;
   struct _Entry entry;
   struct StoredProperty *sp;
   struct _Automatic automatic;
   char *data;
   LONG *p;
   int error, n, i;

   PropChunk(iff, ID_FTXT, ID_AUTO);
   StopChunk(iff, ID_FTXT, ID_CHRS);

   error = ParseIFF(iff, IFFPARSE_SCAN);

/* Read AUTO entries first */
   sp = FindProp(iff, ID_FTXT, ID_AUTO);
   n = AUTO_SIZE;
   if (sp -> sp_Size && sp -> sp_Size % n)
      myMsg1("auto sizes don't match. Old version? Doing my best...");

   p = (LONG *) sp -> sp_Data;
   for (i = 0; i < sp -> sp_Size; i += n) {
      automatic.first.ds_Days = *p++;
      automatic.first.ds_Minute = *p++;
      automatic.first.ds_Tick = *p++;
      automatic.last.ds_Days = *p++;
      automatic.last.ds_Minute = *p++;
      automatic.last.ds_Tick = *p++;
      automatic.repeatDays = *p++;
      automatic.repeatWeeks = *p++;
      automatic.repeatMonths = *p++;
      automatic.repeatYears = *p++;
      memcpy(automatic.amount, p, AMOUNTL); p += (AMOUNTL/ sizeof(p));
      memcpy(automatic.transaction,p, TRANSACTIONL);p += (TRANSACTIONL/sizeof(p));
      memcpy(automatic.imputation, p,IMPUTATIONL); p += (IMPUTATIONL / sizeof(p));
      DB_AddEntry((DataBase) entries -> automatic, & automatic);
   }

/* And then the database itself */
   cn = CurrentChunk(iff);
   n = ENTRY_SIZE;
   if (cn -> cn_Size && cn -> cn_Size % n)
      myMsg1("entry sizes don't match. Old version? Doing my best...");

   for (i=0; i < cn -> cn_Size; i += n) {
      struct DateStamp *ds = & entry.date;

      ReadChunkBytes(iff, & ds -> ds_Days, sizeof(LONG));
      ReadChunkBytes(iff, & ds -> ds_Minute, sizeof(LONG));
      ReadChunkBytes(iff, & ds -> ds_Tick, sizeof(LONG));

      ReadChunkBytes(iff, & entry.transaction, TRANSACTIONL);
      ReadChunkBytes(iff, & entry.amount, AMOUNTL);
      ReadChunkBytes(iff, & entry.checkNumber, CHECKNUMBERL);
      ReadChunkBytes(iff, & entry.imputation, IMPUTATIONL);
      ReadChunkBytes(iff, & entry.reason, REASONL);
      ReadChunkBytes(iff, & entry.validated, 4);
      addEntryDatabase(entries, & entry, 0, 0);  /* not a fake entry */
   }
}

static void
readIFFFile(Gui_t gui, Entries entries, char *file)
{
   BPTR f;
   struct IFFHandle *iff = NULL;

   if (file)
      strcpy(FileName, file);
   else
      getFileName(gui,FileName,getString(MSG_OPEN_FILE), FileName, Pattern);

   f = Open(FileName, MODE_OLDFILE);
   if (f == NULL) {
      myMsg2("couldn't open", FileName);
      return;
   }

   iff = AllocIFF();

   if (! iff) {
      myMsg1("couldn't alloc iff");
      return;
   }

   iff -> iff_Stream = f;

   InitIFFasDOS(iff);

   if (OpenIFF(iff, IFFF_READ)) {
      myMsg1("couldn't OpenIFF(READ)");
      return;
   }

   readDatabaseFile(entries, iff);
   CloseIFF(iff);
   Close(iff -> iff_Stream);
   FreeIFF(iff);
}

static void
writeDatabaseFile(Entries entries, struct IFFHandle *iff)
{
   Automatic nextFreeAuto = NULL;
   Entry nextFreeEntry = NULL;
   DataBase db;
   int j;

   PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN);

/* Write the automatic entries */
   PushChunk(iff, 0, ID_AUTO, IFFSIZE_UNKNOWN);
   j = 0;
   db = (DataBase) entries -> automatic;
   DB_Rewind(db);
   while ((nextFreeAuto = DB_NextEntry(db))) {
      struct DateStamp *ds;

   /* skip to the next free entry */
      ds = & nextFreeAuto -> first;
      WriteChunkBytes(iff, & ds -> ds_Days, sizeof(LONG));
      WriteChunkBytes(iff, & ds -> ds_Minute, sizeof(LONG));
      WriteChunkBytes(iff, & ds -> ds_Tick, sizeof(LONG));

      ds = & nextFreeAuto -> last;
      WriteChunkBytes(iff, & ds -> ds_Days, sizeof(LONG));
      WriteChunkBytes(iff, & ds -> ds_Minute, sizeof(LONG));
      WriteChunkBytes(iff, & ds -> ds_Tick, sizeof(LONG));

      WriteChunkBytes(iff, & nextFreeAuto -> repeatDays, sizeof(ULONG));
      WriteChunkBytes(iff, & nextFreeAuto -> repeatWeeks, sizeof(ULONG));
      WriteChunkBytes(iff, & nextFreeAuto -> repeatMonths, sizeof(ULONG));
      WriteChunkBytes(iff, & nextFreeAuto -> repeatYears, sizeof(ULONG));
      WriteChunkBytes(iff, nextFreeAuto -> amount, AMOUNTL);
      WriteChunkBytes(iff, nextFreeAuto -> transaction, TRANSACTIONL);
      WriteChunkBytes(iff, nextFreeAuto -> imputation, IMPUTATIONL);
   }
   PopChunk(iff);   /* AUTO */

/* Write the regular database */
   PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN);
   j = 0;
   db = (DataBase) entries -> list;
   DB_Rewind(db);
   while ((nextFreeEntry = DB_NextEntry(db))) {
      struct DateStamp *ds = & nextFreeEntry -> date;

      if (! nextFreeEntry -> fake) {  /* only save regular entries, not auto */
         WriteChunkBytes(iff, & ds -> ds_Days, sizeof(LONG));
         WriteChunkBytes(iff, & ds -> ds_Minute, sizeof(LONG));
         WriteChunkBytes(iff, & ds -> ds_Tick, sizeof(LONG));

         WriteChunkBytes(iff, nextFreeEntry -> transaction, TRANSACTIONL);
         WriteChunkBytes(iff, nextFreeEntry -> amount, AMOUNTL);
         WriteChunkBytes(iff, nextFreeEntry -> checkNumber, CHECKNUMBERL);
         WriteChunkBytes(iff, nextFreeEntry -> imputation, IMPUTATIONL);
         WriteChunkBytes(iff, nextFreeEntry -> reason, REASONL);
         WriteChunkBytes(iff, & nextFreeEntry -> validated, 4);
      }
   }
   PopChunk(iff);   /* CHRS */

   PopChunk(iff);   /* FTXT */
}

void
openFile(Gui_t gui, Entries entries, char *file)
{
   FILE *f;
   int result;
   char *backup, *fileToOpen;

   backup = crashFileName(FileName);
   f = fopen(backup, "r");
   if (f) {
      result = showRequester(gui,
                              getString(MSG_WINDOW_CRASH_TITLE),
                              getString(MSG_WINDOW_CRASH_BODY),
                              getString(MSG_YES_OR_NO));
      if (result)
         fileToOpen = backup;
      else
         fileToOpen = file;
   }
   else
      fileToOpen = file;

/* Delete the crash file before opening the new file only if they're different */
   if (fileToOpen != backup && f != NULL) DeleteFile(backup);

/* Clear the current database before opening the new one */
   clearDataBase(entries);

/* Read the file in */
   readIFFFile(gui, entries, fileToOpen);

/* Mark it as unmodified */
   FileModified = 0;

/* Redisplay everything */
   openAutomaticWindow(gui, entries);
   openGetentryWindow(gui, entries);
   fullDisplayList(gui, entries);
}

void
saveFile(Entries entries, char *filename)
{
   BPTR f;
   struct IFFHandle *iff = NULL;

   f = Open(filename, MODE_NEWFILE);
   if (f == NULL) {
      myMsg3("couldn't open ", FileName, IoErr());
      return;
   }

   iff = AllocIFF();

   if (! iff) {
      myMsg1("couldn't alloc iff");
      return;
   }

   iff -> iff_Stream = f;

   InitIFFasDOS(iff);

   if (OpenIFF(iff, IFFF_WRITE)) {
      myMsg1("couldn't OpenIFF(WRITE)");
      return;
   }

   writeDatabaseFile(entries, iff);
   CloseIFF(iff);
   Close(iff -> iff_Stream);
   FreeIFF(iff);
}

/*******************************************************************
 * Options file
 *******************************************************************/
 
void
writeOptionsFile(Gui_t gui)
{
   FILE *f;
   char *name = (CL_Struct.optionsFile ? CL_Struct.optionsFile : OPTIONSFILENAME);
   char *string;
   struct WindowDesc wd;
   int isOpen;

   f = fopen(name, "w");
   if (! f) {
      myMsg2("couldn't open the configuration file", name);
      return;
   }

/* Write the comment lines */
/*
   string = getString(MSG_PREFERENCES_HELP);
   fputs(string, f);
   string = getString(MSG_PREFERENCES_HELP2);
   fputs(string, f);
*/

   get(gui -> mainWindowObject, MUIA_Window_Open, & isOpen);
   fprintf(f, "mainwindow %d\n", isOpen);
   get(gui -> automaticWindowObject, MUIA_Window_Open, & isOpen);
   fprintf(f, "automatic %d\n", isOpen);
   get(gui -> listWindowObject, MUIA_Window_Open, & isOpen);
   fprintf(f, "list %d\n", isOpen);
   get(gui -> getEntryWindowObject, MUIA_Window_Open, & isOpen);
   fprintf(f, "getEntry %d\n", isOpen);
   fprintf(f, "\n");
   fprintf(f, "autosave %d\n", OF_Struct.autoSave);
   fprintf(f, "\n");
   fprintf(f, "dateFormat %s\n", OF_Struct.dateFormat);
   fprintf(f, "defaultDate %s\n", OF_Struct.defaultDate);
   fprintf(f, "\n");
   fprintf(f, "displayFormat %s\n", OF_Struct.displayFormat);
   fprintf(f, "printFormat %s\n", OF_Struct.printFormat);
   fprintf(f, "exportFormat %s\n", OF_Struct.exportFormat);

   fclose(f);
}

void
writeTextFile(Entries entries, char *file, struct StringAspect *sa)
{
   FILE *f;
   Entry entry;
   DataBase db;
   char *string;
 
  f = fopen(file, "w");
   if (f == NULL) {
      myMsg2("couldn't open to write", file);
      return;
   }

/*@@@*/
   buildDataBase(entries);
   db = (DataBase) entries -> list;
   DB_Rewind(db);
   while ((entry = DB_NextEntry(db)) != NULL) {
      string = returnFormattedString(& ExportFormatStructure, entry);
      fprintf(f, "%s\n", string);
   }

   fclose(f);
}

void
readOptionsFile()
{
   FILE *f;
   int isOpen;
   char *p, keyword[80], *pk, buffer[256];
   char *name = (CL_Struct.optionsFile ? CL_Struct.optionsFile : OPTIONSFILENAME);

   f = fopen(name, "r");
   if (! f) {
      myMsg2("couldn't open the configuration file", name);
      goto close;
   }

   while (1) {
      p = fgets(buffer, 256, f);
      if (p == NULL) goto close;
      if (*p == '#' || *p == '\n') continue;      /* skip comments */
      if (feof(f)) goto close;
      pk = keyword;
      while (! isblank(*p)) *pk++ = *p++;
      *pk++ = '\0';               /* now the keyword is recorded */
      while (isblank (*p)) p++;  /* and p is on the first data field */

      if (stricmp(keyword, "mainwindow") == 0 ||
          stricmp(keyword, "list") == 0 ||
          stricmp(keyword, "getentry") == 0 ||
          stricmp(keyword, "automatic") == 0) {

         sscanf(p, " %d\n", & isOpen);
         if (keyword[0] == 'm' || keyword[0] == 'M') {
            OF_Struct.mainWindow.isOpen = isOpen;
         }
         else if (keyword[0] == 'l' || keyword[0] == 'L') {
            OF_Struct.list.isOpen = isOpen;
         }
         else if (keyword[0] == 'g' || keyword[0] == 'G') {
            OF_Struct.getEntry.isOpen = isOpen;
         }
         else if (keyword[0] == 'a' || keyword[0] == 'A') {
            OF_Struct.automatic.isOpen = isOpen;
         }
      }

      else if (stricmp(keyword, "autosave") == 0) {
         OF_Struct.autoSave = atoi(p);
      }

      else if (stricmp(keyword, "defaultdate") == 0) {
         char *df;
         OF_Struct.defaultDate = df = (char *) malloc(strlen(p));
         while (*p && *p != '\n') *df++ = *p++; /* to prevent an '\n' in the date */
         *df = '\0';
      }

      else if (stricmp(keyword, "dateformat") == 0) {
         char *df;
         OF_Struct.dateFormat = df = (char *) malloc(strlen(p));
         while (*p && *p != '\n') *df++ = *p++; /* to prevent an '\n' in the date */
         *df = '\0';
      }

      else if (stricmp(keyword, "printformat") == 0 ||
               stricmp(keyword, "displayformat") == 0 ||
               stricmp(keyword, "exportformat") == 0) {
         if (keyword[0] == 'p')  PrintFormatStructure = *parseFormatString(p);
         else if (keyword[0] == 'e') ExportFormatStructure = *parseFormatString(p);
         else {
            parseListEntry(p, DisplayEntryFormat);
            DisplayFormatStructure = *parseFormatString(p);
         }
      }


      else myMsg2("unknown keyword '%s'", keyword);
      while (*p && *p != '\n') p++;
   }
close:
   fclose(f);
}


