/*
 * common.c
 */

/* $Id: common.c,v 1.5 1993/09/03 00:17:28 beust Exp beust $ */

#define STRINGARRAY

#include "common.h"

BOOL FileModified;              /* True if file was modified */
char FileName[128];            /* last accessed filename  */
char ExportFileName[128];     /* exported file name  */
char PrintFileName[128];     /* print filename */
char Pattern[128];          /* pattern for file requester  */
char ExportPattern[128];   /* pattern for export requester  */
char PrintPattern[128];   /* pattern for print requester  */


/* The console window used for output */
FILE *BankerWindow = stderr;

struct IconBase *IconBase = NULL;
struct IFFParseBase *IFFParseBase = NULL;
struct GadToolsBase *GadToolsBase = NULL;
struct UtilityBase *UtilityBase = NULL;
struct IntuitionBase* IntuitionBase = NULL;
struct GfxBase* GfxBase = NULL;
struct LocaleBase *LocaleBase = NULL;
struct DOSBase *DOSBase= NULL;

struct Locale *MyLocale = NULL;
struct Catalog *MyCatalog = NULL;

Entries MainList;

/* Used to parse date with respect to locale */
#define LOCALEDATEBUFFERSIZE 100
char LocaleDateBuffer[LOCALEDATEBUFFERSIZE];
int LocaleDateBufferPos = 0;

/* Command line */
char *CL_Usage = "Language/K,File/K,OptionsFile/K";
struct CommandLineStruct CL_Struct;

/* Options file */
struct OptionsFileStruct OF_Struct;

/* Format variables */
struct StringAspect PrintFormatStructure, ExportFormatStructure,
                    DisplayFormatStructure;

struct ListEntry DisplayEntryFormat[MAX_FIELDS];

/* Automatic backup */
struct timerequest *AutosaveTR = NULL;    /* autosave time request */
LONG AutosaveSignal;
struct MsgPort *AutosavePort = NULL;

/* Asl */
struct AslBase *AslBase = NULL;

/* Gadtools */
struct Screen *MyScreen;
struct Window *GlobalWindow, *BDWindow,   /* backdrop window */
              *GetentryWindow, *AutomaticWindow, *ListWindow;
char ScreenTitle[128];

int UniqueID = 0;

/***********
 * Private
 ***********/


void
freeEntry(Entry entry)
{
}

void
freeAutomatic(Automatic aut)
{
}


char
getSignificantCharacter(int n)
{
   char result, *s = GetCatalogStr(MyCatalog, n, AppStrings[n].as_Str), *p;

   result = s[0];
   p = s;
   while (*p && *p != '_') p++;
   if (*p && *(p+1)) result = *(p+1);
   result = ToLower(result);

   return result;
}

char *
getString(int n)
{
   char *result = (char *) GetCatalogStr(MyCatalog, n, AppStrings[n].as_Str), *p;
   result = strdup(result);
   p = result;
   while (*p && *p != '_') p++;
   if (*p == '_') {
     while (*p != '\0') { *p = *(p+1); p++; }
   }
   return result;
/*
   char *result;

   STRPTR s;

   s = AppStrings[n].as_Str;
   if (MyCatalog)
      result = GetCatalogStr(MyCatalog, n, s);
   else
      result = s;

   return result;
*/
}

void
openLocale(char *locale)
{
   if ((LocaleBase = (struct LocaleBase *)
	                OpenLibrary("locale.library", 38L)) == NULL) {
      myMsg1("couldn't open locale.library, using default language");
   }
   else {
      if (LocaleBase == NULL ||
          (LocaleBase && LocaleBase -> lb_LibNode.lib_Revision < 27)) {
         myMsg1(getString(MSG_BETA_VERSION));
      }

      MyCatalog = OpenCatalog(NULL, "banker.catalog",
			      OC_BuiltInLanguage, "english",
/*
  OC_Language, locale,
*/
			      TAG_DONE);
      printf("string = '%s'\n",
	     GetCatalogStr(MyCatalog, MSG_MENU_FILE, "engllish"));
      if (MyCatalog == NULL)
	myMsg3("OpenCatalog NULL", "IoErr():", IoErr());
   } /* else */
}

/*********
 * General
 *********/

void
myMsg1(char *s1)
{
   fprintf(BankerWindow, "%s\n", s1);
}

void
myMsg2(char *s1, char *s2)
{
   fprintf(BankerWindow, "%s : %s\n", s1, s2);
}

void
myMsg3(char *s1, char *s2, int n)
{
   fprintf(BankerWindow, "%s : %s (%d)\n", s1, s2, n);
}

void
closeAll()
{

/* Delete autosave stuff */
   if (AutosaveTR) {
      if (OF_Struct.autoSave > 0) AbortIO(AutosaveTR);
      CloseDevice((struct IORequest *) AutosaveTR);
      DeleteExtIO((struct IORequest *) AutosaveTR);
   }
   DeletePort(AutosavePort);

/* Delete the crash file */
   DeleteFile(crashFileName(FileName));

/* Close the screen */
   if (MyScreen) CloseScreen(MyScreen); MyScreen = NULL;

/* And the possible output window */
   if (BankerWindow != stderr)
      fclose(BankerWindow);

/* And all the libraries */
   if (AslBase) CloseLibrary(AslBase); AslBase = NULL;
   if (GfxBase) CloseLibrary(GfxBase); GfxBase = NULL;
   if (IFFParseBase) CloseLibrary(IFFParseBase); IFFParseBase = NULL;
   if (GadToolsBase) CloseLibrary(GadToolsBase); GadToolsBase = NULL;
   if (UtilityBase) CloseLibrary(UtilityBase); UtilityBase = NULL;
   if (IconBase) CloseLibrary(IconBase); IconBase = NULL;

}

void
openLibraries(Entries *entries)
{
   ULONG error;

   DOSBase = (struct DOSBase *) OpenLibrary("dos.library", 37L);
   if (! DOSBase) {
      myMsg1("couldn't open dos.library");
      closeAll();
   }

   IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 37L);
   if (! IntuitionBase) {
      myMsg1("couldn't open intuition.library");
      closeAll();
   }

   LocaleBase = (struct LocaleBase *) OpenLibrary("locale.library", 38L);
   if (! LocaleBase) {
      myMsg1("couldn't open locale.library, using default language");
   }
   else {
      if (LocaleBase -> lb_LibNode.lib_Revision < 27) {
	 myMsg1(getString(MSG_BETA_VERSION));
      }
      MyCatalog = OpenCatalog(NULL, "banker.catalog",
			      OC_BuiltInLanguage, "english",
			      TAG_DONE);
      if (MyCatalog == NULL)
	myMsg3("OpenCatalog NULL", "IoErr():", IoErr());
   } /* else */

   AslBase = (struct AslBase *) OpenLibrary("asl.library", 37L);
   if (! AslBase) {
      myMsg1("couldn't open asl.library");
      closeAll();
   }

   GfxBase= (struct GfxBase*) OpenLibrary("graphics.library", 37L);
   if (! GfxBase) {
      myMsg1("couldn't open graphics.library");
      closeAll();
   }

   IFFParseBase = (struct IFFParseBase *) OpenLibrary("iffparse.library", 0L);
   if (! IFFParseBase) {
      myMsg1("couldn't open iffparse.library");
      closeAll();
   }

   GadToolsBase = (struct GadToolsBase*) OpenLibrary("gadtools.library", 37L);
   if (! GadToolsBase) {
      myMsg1("couldn't open gadtools.library");
      closeAll();
   }

   UtilityBase= (struct UtilityBase*) OpenLibrary("utility.library", 37L);
   if (! UtilityBase) {
      myMsg1("couldn't open utility.library");
      closeAll();
   }

   IconBase= (struct IconBase*) OpenLibrary("icon.library", 37L);
   if (! IconBase) {
      myMsg1("couldn't open icon.library");
      closeAll();
   }

   NEW(*entries, struct _Entries);

   (*entries) -> list = (Entry *)
     DB_NewDataBase(sizeof(struct _Entry), freeEntry);
   (*entries) -> automatic =(Automatic*)
     DB_NewDataBase(sizeof(struct _Automatic), freeAutomatic);

/* Global variables changing through the execution */
   strcpy(FileName, getString(MSG_NO_FILE_LOADED));
   ExportFileName[0] = '\0';
   strcpy(PrintFileName, "PRT:");
   strcpy(Pattern, "#?.bank");
   strcpy(PrintPattern, "");
   strcpy(ExportPattern, "");

/* Allocate structures for autosave */
   AutosavePort = (struct MsgPort *) CreatePort(0, 0);
   if (! AutosavePort) {
      myMsg1("*** couldn't allocate autosave port");
      closeAll();
   }

   AutosaveSignal = 1 << AutosavePort -> mp_SigBit;

   AutosaveTR = (struct timerequest *)
                     CreateExtIO(AutosavePort, sizeof(struct timerequest));

   if (! AutosaveTR) {
      myMsg1("*** couldn't allocate time request");
      closeAll();
   }

   error = OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest*) AutosaveTR, 0L);
   if (error) {
      myMsg3("*** couldn't open device", TIMERNAME, error);
      closeAll();
   }

/* Initialize windows */
   GetentryWindow = NULL;
   AutomaticWindow = NULL;
   ListWindow = NULL;

/* The file hasn't been modified yet */
   FileModified = 0;

/* Defaults values for options */
   OF_Struct.mainWindow.isOpen = TRUE;
   OF_Struct.list.isOpen = FALSE;
   OF_Struct.automatic.isOpen = FALSE;
   OF_Struct.getEntry.isOpen = FALSE;

   OF_Struct.dateFormat = "%D";      /* default extended date %m/%d/%y */
   OF_Struct.defaultDate = "CDN";   /* default poor-man's-date mm-dd-yy */
   OF_Struct.autoSave = 1;       /* 1 minute */

   OF_Struct.displayFormat = "date amount transaction \"F\"";   /* default %m/%d/%y */
   OF_Struct.exportFormat = "date amount transaction \"F\"";   /* default %m/%d/%y */
   OF_Struct.printFormat = "date amount transaction \"F\"";   /* default %m/%d/%y */
   readOptionsFile();
}

char *
crashFileName(char *filename)
{
   char *result = (char *) malloc(strlen(filename) + 10);
   strcpy(result, filename);
   strcat(result, ".crash");
   return result;
}

void
readEnvironment(int argc, char **argv)
{
   struct CSource cs;
   struct RDArgs rda, *rda2;
   struct CommandLineStruct result;
   char language[64];

   result.filename = NULL;
   result.language = NULL;
   result.optionsFile= NULL;

   if (argc) {    /* called from CLI */
      BankerWindow = stderr;
      rda2 = ReadArgs(CL_Usage, & result, NULL);

   /* We have to copy any returned value, its reference is not safe */
      if (result.filename) {
         CL_Struct.filename = (char *) malloc(strlen(result.filename) + 2);
         strcpy(CL_Struct.filename, result.filename);
      }

      if (result.language) {
         CL_Struct.language = (char *) malloc(strlen(result.language) + 2);
         strcpy(CL_Struct.language, result.language);
      }

      if (result.optionsFile) {
         CL_Struct.optionsFile = (char *) malloc(strlen(result.optionsFile) + 2);
         strcpy(CL_Struct.optionsFile, result.optionsFile);
      }

      FreeArgs(rda2);
   }

   else {    /* called from Workbench */
      struct WBStartup *wbs;
      struct WBArg *wba;
      struct DiskObject *dobj;
      char **array, *s;
      int i;

      BankerWindow = fopen(BANKEROUTPUTWINDOW, "w");

      wbs = (struct WBStartup *) argv;       /* cast argc into WBStartup */
      wba = & wbs -> sm_ArgList[0];          /* retrieve argv[0] (ourselves) */
      myMsg2("getting disk object", wba -> wa_Name);
      dobj = GetDiskObject(wba -> wa_Name);  /* and find the disk object */

   /* Retrieve all the tooltypes for myself */
      array = (char **) dobj -> do_ToolTypes;
      if (array) {               /* there are tooltypes */
         if ((s = FindToolType(array, "FILE"))) {
            CL_Struct.filename = strdup(s);
         }
         if ((s = FindToolType(array, "OPTIONSFILE"))) {
            CL_Struct.optionsFile = strdup(s);
         }
         if ((s = FindToolType(array, "LANGUAGE"))) {
            CL_Struct.language = strdup(s);
         }
      }

   /* Parse all the files that were possibly multi-selected with me */
      for (i = 1; i < wbs -> sm_NumArgs; i++) {    /* read all the icons */
         wba = & wbs -> sm_ArgList[i];
         if (wba -> wa_Lock != NULL && *wba -> wa_Name)
            CurrentDir(wba -> wa_Lock);    /* change to this directory */
      }
   }

   MyLocale = OpenLocale(CL_Struct.language);

}

void __saveds __asm putCharFunc(register __a0 struct Hook *h,
                                register __a2 void *object,
                                register __a1 char c)
{
    if (LocaleDateBufferPos < LOCALEDATEBUFFERSIZE)
      ((char *)h->h_Data)[LocaleDateBufferPos++] = c;
}

struct Hook putCharHook = {
    {NULL, NULL},
    (unsigned long (*) ()) putCharFunc,
    NULL,
    LocaleDateBuffer
};

unsigned long __saveds __asm getCharFunc(register __a0 struct Hook *h,
                                register __a2 void *object,
                                register __a1 void *locale)
{
   unsigned long result;

    if (LocaleDateBufferPos < LOCALEDATEBUFFERSIZE)
      result = (((char *)h->h_Data)[LocaleDateBufferPos++]) & 0xff;

   return result;
}

struct Hook getCharHook = {
    {NULL, NULL},
    (unsigned long (*) ()) getCharFunc,
    NULL,
    LocaleDateBuffer
};


char *
myDateToStr(struct DateStamp *ds)
{
   int fullFunctionalities = 1;
   STRPTR format;
   char *result, time[64];
   struct DateTime dt;
   BOOL bool;

/*
      format = MyLocale -> loc_ShortDateFormat;  /* short date format *
*/
   if (LocaleBase == NULL) fullFunctionalities = 0;
   if (LocaleBase && LocaleBase -> lb_LibNode.lib_Revision < 27)
      fullFunctionalities = 0;

   if (fullFunctionalities) {
      format = OF_Struct.dateFormat;
      LocaleDateBufferPos = 0;
      FormatDate(MyLocale, format, ds, &putCharHook);
      sprintf(time,"%12s", LocaleDateBuffer);   /* Reformatting */
      strcpy(LocaleDateBuffer, time);
      result = LocaleDateBuffer;
   }
   else {
      if (ds -> ds_Days == 0) return "";
      result = (char *) malloc(64);
      dt.dat_Stamp = *ds;
      dt.dat_StrDate = result;
      dt.dat_StrTime = NULL;
      dt.dat_StrDay = NULL;
      dt.dat_Format = FORMAT_CDN;
      if (STREQUAL(OF_Struct.defaultDate, "DOS")) dt.dat_Format = FORMAT_DOS;
      else if (STREQUAL(OF_Struct.defaultDate, "CDN")) dt.dat_Format = FORMAT_CDN;
      else if (STREQUAL(OF_Struct.defaultDate, "INT")) dt.dat_Format = FORMAT_INT;
      else if (STREQUAL(OF_Struct.defaultDate, "USA")) dt.dat_Format = FORMAT_USA;
      else {
         myMsg2("Unknown defaultDate keyword (using CDN) : ", OF_Struct.defaultDate);
         dt.dat_Format = FORMAT_CDN;
      }
      dt.dat_Flags = NULL; /* could be DTF_SUBST to have Monday when possible */
      bool = DateToStr(& dt);
      if (! bool) result = "ErrDateStr";
   }
   while (*result == ' ') result++;
   return result;
}


struct DateStamp *
myStrToDate(char *dateString)
{

   int fullFunctionalities = 1;
   struct DateTime *dt;
   struct DateStamp *result;
   char *format;
   int bool;

   dt = (struct DateTime *) malloc(sizeof *dt);
   result = & dt -> dat_Stamp;

   result -> ds_Days = 0;
   result -> ds_Minute = 0;
   result -> ds_Tick = 0;

   if (dateString == NULL || (dateString && *dateString == '\0'))
      return result;

   if (LocaleBase == NULL) fullFunctionalities = 0;
   if (LocaleBase && LocaleBase -> lb_LibNode.lib_Revision < 27)
      fullFunctionalities = 0;

   if (! fullFunctionalities) {
      if (STREQUAL(OF_Struct.defaultDate, "CDN")) dt -> dat_Format = FORMAT_CDN;
      else if (STREQUAL(OF_Struct.defaultDate, "INT")) dt->dat_Format= FORMAT_INT;
      else if (STREQUAL(OF_Struct.defaultDate, "DOS")) dt->dat_Format= FORMAT_DOS;
      else if (STREQUAL(OF_Struct.defaultDate, "USA")) dt->dat_Format= FORMAT_USA;
      else {
         myMsg2("unknown defaultdate token (using CDN)", OF_Struct.defaultDate);
         dt -> dat_Format = FORMAT_CDN;
      }
      dt -> dat_StrDay = NULL;
      dt -> dat_StrDate = dateString;
      dt -> dat_StrTime = NULL;
      dt -> dat_Flags = 0;
      StrToDate(dt);
      result = & dt -> dat_Stamp;
   }
   else {
      format = OF_Struct.dateFormat;
      LocaleDateBufferPos = 0;
      strcpy(LocaleDateBuffer, dateString);
      bool = ParseDate(MyLocale, result, format, & getCharHook);
   }

   return result;

}

