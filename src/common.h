/*
 * common.h
 */

/* $Id: common.h,v 1.4 1993/08/28 19:39:39 beust Exp $ */

#include <stdio.h>
#include <exec/types.h>
#include <strings.h>
#include <libraries/mui.h>


/* Libraries definitions */
#include <libraries/locale.h>
#include <libraries/gadtools.h>

/* Graphics */
#include <graphics/displayinfo.h>
#include <intuition/gadgetclass.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

/* Prototypes */
#include <clib/locale_protos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/dos_protos.h>
#include <clib/iffparse_protos.h>
#include <clib/utility_protos.h>
#include <clib/wb_protos.h>
#include <clib/icon_protos.h>
#include <clib/muimaster_protos.h>

/* Pragmas */
#include <pragmas/muimaster_pragmas.h>

/* Personal includes */
#include "locale_strings.h"
#include "database.h"

/* Icon */
extern struct IconBase *IconBase;

/* Locale */
extern struct LocaleBase *LocaleBase;
extern struct Locale *MyLocale;
extern struct Catalog *MyCatalog;

/* General definitions */
#define NEW(v, t) v = (t *) malloc(sizeof(t))
#define STREQUAL(s1, s2) (strcmp(s1, s2) == 0)

#define isblank(c) (c == ' ' || c == '\t')
#define myAssert(p)
#ifdef cedric
#define D(x) { printf("%s:%d--- ", __FILE__, __LINE__); x; }
#else
#define D(x)
#endif


/* The main structure */
#define ENTRYFORMATSTRING "%c %8s|%8s| %-6.2f |%8s|%8s|%35s"
#define AUTOFORMATSTRING  "%c %8s    %3d %3d %3d %3d   %30s"

#define DATEL sizeof(struct DateStamp)
#define TRANSACTIONL 80
#define AMOUNTL 32
#define CHECKNUMBERL 20
#define IMPUTATIONL 80
#define REASONL 80

typedef struct _Automatic {
   char *formattedString;            /* the entry once it is formatted */
   struct DateStamp first;          /* first occurring for this automatic entry */
   struct DateStamp last;               /* last... */
   ULONG repeatDays;                   /* repeat each days */
   ULONG repeatWeeks;                 /* ... weeks */
   ULONG repeatMonths;               /* ... months */
   ULONG repeatYears;               /* ... years */
   char transaction[REASONL];      /* reason to give */
   char imputation[IMPUTATIONL];  /* category where this automatic falls into */
   char amount[AMOUNTL];         /* the amount of the transaction */
} *Automatic;

typedef struct _Entry {
   char *formattedString;            /* the entry once it is formatted */
   struct DateStamp date;
   char transaction[TRANSACTIONL];
   char checkNumber[CHECKNUMBERL];
   char amount[AMOUNTL];          /* the amount of the transaction */
   char imputation[IMPUTATIONL]; /* category where this automatic falls into */
   char reason[REASONL];        /* reason to give */
   char validated;             /* boolean actually */
   Automatic generatedBy;     /* if this entry comes from an automatic */
   char fake;                /* if true, this entry was generated automatically */
   char pad[3];             /* pad the rest for a long aligned struct */
         
} *Entry;

#define ENTRY_SIZE DATEL+TRANSACTIONL+AMOUNTL+CHECKNUMBERL+IMPUTATIONL+REASONL+4
#define AUTO_SIZE sizeof(struct DateStamp) * 2 + sizeof(ULONG) * 4 +\
                  AMOUNTL + IMPUTATIONL + REASONL

typedef struct _Entries {
   Entry *list;
   Automatic *automatic;
} *Entries;

typedef float TotalType;

/* This structure to hold the names of my listview */
/* Since a list can be displayed several times, I must do smart allocation */
/* here (e.g. see if there's enough room in the previous list for the new */
/* one, else allocate another one). Initialize it to NULL and pass it to */
/* buildList() */ 

struct SmartAlloc {
  int count;
  char **strings;
};

/*****************
 * The format structures
 *****************/

enum Fields {
   DATEFIELD = 1 << 0, AMOUNTFIELD = 1 << 1, TRANSACTIONFIELD = 1 << 2,
   REASONFIELD = 1 << 3, CHECKNUMBERFIELD = 1 << 4, IMPUTATIONFIELD = 1 << 5
};

/*
 ** A column in the list view is represented by the following structure
 ** The first field is the list of the selected fields, coded as
 ** a string (e.g. "\01\03" = DATE REASON. numberOfFields would be 2 for
 ** this column. 
 */


#define MAX_FIELDS 6
#define MAX_COLUMNS 20


struct ColumnListEntry {
   char fields[MAX_FIELDS + 1];       /* the fields for this column */
   int numberOfFields;
   char *format;
};

struct ListEntry {
   int numberOfColumns;
   struct ColumnListEntry col[MAX_COLUMNS];
   char *after;
};

extern struct ListEntry DisplayEntryFormat[];     /* common.c */

struct StringAspect {
   enum Fields field[6];         /* what field */
   char *format[6];              /* string in printf format */
   char *after;                  /* the possible trailing string */
   int numberOfFields;           /* number of fields the user wants */
};

extern struct StringAspect PrintFormatStructure,
                           ExportFormatStructure,
                           DisplayFormatStructure;    /* common.c */

/*********
 * The GUI structure
 *********/

/*
typedef void (*MenuCallbackPointer_t)() ;
*/

#define MENU_ITEM_COUNT 20
#define MENU_CALLBACK(f) void f(Gui_t gui, Entries entries)

typedef struct _Gui {
  APTR app;
  APTR mainWindowObject, getEntryWindowObject,
       automaticWindowObject, clearWindowObject, listWindowObject;
  APTR geDate, geCheckNumber, geTransaction, geImputation, geAmount,
       geReplace, geReason;
  APTR auFrom, auTo, auAmount, auDays, auWeeks, auMonths, auYears,
       auReason, auImputation, auListviewObject, auListObject,
       auNew, auAdd, auReplace, auDelete;
  APTR liCycleObject, liListviewObject, liListObject,
       liValidatedObject, liTotalObject;

  struct SmartAlloc entrySA, autoSA;
  void **menuFunctions;
} *Gui_t;

extern struct _Gui Gui;    /* gui.c */
extern struct Library *MUIMasterBase;      /* gui.c */



/*********
 * Time variables
 *********/

extern struct timerequest *AutosaveTR; /* (common.c) */
extern LONG AutosaveSignal;            /* (common.c) */
extern struct MsgPort *AutosavePort;   /* (common.c) */


/*********
 * IFF defines
 *********/

#define ID_FTXT MAKE_ID('F','T','X','T')
#define ID_CHRS MAKE_ID('C','H','R','S')
#define ID_AUTO MAKE_ID('A','U','T','O')

/*********
 * The output
 *********/

#define BANKEROUTPUTWINDOW "CON:0/10/640/100"
extern FILE *BankerWindow;    /* common.c */


/*********
 * Command line parsing
 *********/

/* Command line structure (passed to ReadArgs()) */
#define OPTIONSFILENAME "s:.bankerconfig"    /* default name for the config file */

struct CommandLineStruct {
   char *language;
   char *filename;
   char *optionsFile;
};
extern struct CommandLineStruct CL_Struct;  /* (common.c) */
extern char *CL_Usage; /* (common.c) */

/* The fields in the options file */
struct WindowDesc {
   BOOL isOpen;
};


/*********
 * Options file parsing
 *********/

struct OptionsFileStruct {
   struct WindowDesc mainWindow;
   struct WindowDesc automatic;
   struct WindowDesc getEntry;
   struct WindowDesc list;
   int autoSave;
   char *defaultDate;
   char *dateFormat;
   char *displayFormat;
   char *exportFormat;
   char *printFormat;
};
extern struct OptionsFileStruct OF_Struct;  /* (common.c) */

extern int UniqueID;

double atof();
#define STRINGTOTOTAL(s) atof(s)

extern struct _Entry LastSelectedEntry;   /* getentry.c */
extern BOOL CanReplace;     /* getentry.c */

extern Entries MainList;   /* the big Database (main.c) */

extern BOOL FileModified; /* true if file has been modified (common.c) */

/*
 ** common.c
 */

extern BOOL FileModified;              /* True if file was modified */
extern char FileName[128];            /* last accessed filename  */
extern char ExportFileName[128];     /* exported file name  */
extern char PrintFileName[128];     /* print filename */
extern char Pattern[128];          /* pattern for file requester  */
extern char ExportPattern[128];   /* pattern for export requester  */
extern char PrintPattern[128];   /* pattern for print requester  */



/*********
 * MUI
 *********/

#define ID_MAINWINDOW 0x4489
#define ID_GETENTRYWINDOW 0x448a
#define ID_AUTOMATICWINDOW 0x448b
#define ID_LISTWINDOW 0x448c

/*********
 * Prototypes
 *********/


/*
 * gui.c
 */

int
buildGui(Gui_t gs, Entries entries, struct OptionsFileStruct *ofStruct);
/* Build the GUI and main loop for the events */


/*
 * automatic.c
 */

void
openAutomaticWindow(Gui_t gui, Entries entries);
/* Open the automatic window and set the gadgets to default value */

void
clearAutomaticGadgets(Gui_t gui);
/* Set the gadgets to their default values */

void
newAutomatic(Gui_t gui, Entries entries);
/* Clear the gadgets to enter a new automatic */

void
replaceAutomatic(Gui_t gui, Entries entries);
/* Replace the previously selected automatic with the one in the gadgets */

void
addAutomatic(Gui_t gui, Entries entries);
/* Add the automatic in the gadgets to the database */

void
deleteAutomatic(Gui_t gui, Entries entries, Automatic automatic);
/* If automatic == NULL, retrieve the automatic from the gadgets */

void
editAutomatic(Gui_t gui, Automatic automatic);
/* Edit an automatic entry, open the window if necessary */


/*
 * getentry.c
 */

void
openEntryWindow(Gui_t gui, Entries entries);
/* Open the getentry window and set the gadgets to default value */

Entry
getEntryFromGadgets(Gui_t gui);
/* Return the entry currently in the gadgets */

void
clearGetentryGadgets(Gui_t gui);
/* Clear the gadgets */

void
addEntry(Gui_t gui, Entries entries);
/* Add the entry currently in the gadgets to the database */

void
replaceEntry(Gui_t gui, Entries entries);
/* Replace the latest selected entry with the one in the gadgets */

void
editEntry(Gui_t gui, Entry entry);
/* Edit the selected entry */


/*
 * list.c
 */


void
openListWindow(Gui_t gui, Entries entries);
/* Open the list window */

void
simpleDisplayList(Gui_t gui, Entries entry);
/* Display the database in the listview */

void
fullDisplayList(Gui_t gui, Entries entry);
/* Display the database in the listview */

void
handleValidate(Gui_t gui, Entries entries, Entry entry);
/* Validate the specified entry */

void
handleEdit(Gui_t gui, Entries entries, Entry entry);
/* Edit the specified entry, open the getentry window if necessary */

void
handleDelete(Gui_t gui, Entries entries, Entry entry);
/* Delete the specified entry */


/*
 * io.c
 */

void
openFile(Gui_t gui, Entries entries, char *file);

void
saveFile(Entries entries, char *filename);

void
writeOptionsFile(Gui_t gui);
/* Get the window configuration and write it to disk */

void
writeTextFile(Entries entries, char *file, struct StringAspect *sa);

void
readOptionsFile();

/*
 * common.c
 */

void
closeAll();

void
openLibraries(Entries *entries);

char
getSignificantCharacter(int n);

char *
getString(int n);

void
myMsg1(char *s1);

void
myMsg2(char *s1, char *s2);

void
myMsg3(char *s1, char *s2, int n);

char *
crashFileName(char *filename);

void
readEnvironment(int argc, char **argv);
/* Read the command line (or the WBMessage), and take appropriate actions */
/* (open file, etc...) */

struct DateStamp *
myStrToDate(char *dateString);

char *
myDateToStr(struct DateStamp *ds);

/*
 * common2.c
 */

void
addEntryDatabase(Entries entries, Entry entry, char fake, Automatic generatedBy);

TotalType
computeValidatedTotal(Entries list);

TotalType
computeTotal(Entries list);

void
buildDataBase(Entries entries);

char *
returnFormattedString(struct StringAspect *sa, Entry entry);

void
sortList(Entries list);

int
daysInNYears(ULONG days, ULONG n, ULONG year);

int
daysInNMonths(ULONG days, ULONG n, ULONG month, ULONG year);

char *
printEntry(Entries list, int number, char *s);

void
printEntries(Entries list);

struct StringAspect *
parseFormatString(char *string);
/* [[@n &n] keyword string] * */

void
clearDataBase(Entries entries);

void
getFileName(Gui_t gui, char *initial, char *hail, char *result, char *pattern);

int
showRequester(Gui_t gui, char *title, char *text, char *choices);

char *
myPathPart(char *path);

void
buildList(Gui_t gui, Entries entries, BOOL entry, BOOL sorted,
	  struct SmartAlloc *sn);


/*
 * menu.c
 */

void
addToNewMenu(struct NewMenu *nm, Gui_t gui,
	     UBYTE type, STRPTR label, STRPTR shortCut,
	     int id, APTR callback, int n);

MENU_CALLBACK(menuOpen);
MENU_CALLBACK(menuSave);
MENU_CALLBACK(menuSaveAs);
MENU_CALLBACK(menuPrint);
MENU_CALLBACK(menuExport);
MENU_CALLBACK(menuSavePrefs);
