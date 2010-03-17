/*
 * getentry.c
 */

/* $Id: getentry.c,v 1.5 1993/09/03 00:17:28 beust Exp beust $ */

#include "common.h"

/*
static int Validated = 0;
*/
struct _Entry LastSelectedEntry;
BOOL CanReplace = FALSE;    /* true if LastSelectedEntry is valid */

static void
setGetentryGadgetsToEntry(Gui_t gui, Entry entry)
{
   struct DateStamp current;
   int windowOpen;

   get(gui -> getEntryWindowObject, MUIA_Window_Open, & windowOpen);
   if (! windowOpen) {
      set(gui -> getEntryWindowObject, MUIA_Window_Open, TRUE);
    }

   DateStamp(& current);
   if (entry == NULL) {
      set(gui -> geDate, MUIA_String_Contents, myDateToStr(& current));
      set(gui -> geTransaction, MUIA_String_Contents, "");
      set(gui -> geAmount, MUIA_String_Contents, "");
      set(gui -> geCheckNumber, MUIA_String_Contents, "");
      set(gui -> geImputation, MUIA_String_Contents, "");
      set(gui -> geReason, MUIA_String_Contents, "");
   }
   else {
      set(gui -> geDate, MUIA_String_Contents, myDateToStr(& entry -> date));
      set(gui -> geTransaction, MUIA_String_Contents, entry -> transaction);
      set(gui -> geAmount, MUIA_String_Contents, entry -> amount);
      set(gui -> geCheckNumber, MUIA_String_Contents, entry -> checkNumber);
      set(gui -> geImputation, MUIA_String_Contents, entry -> imputation);
      set(gui -> geReason, MUIA_String_Contents, entry -> reason);
   }
}

void
openGetentryWindow(Gui_t gui, Entries entries)
{
   set(gui -> getEntryWindowObject, MUIA_Window_Open, TRUE);
   setGetentryGadgetsToEntry(gui, NULL);
}

Entry
getEntryFromGadgets(Gui_t gui)
{
   Entry result;
   char *dateBuffer, *transactionBuffer, *amountBuffer, *checkNumberBuffer,
        *imputationBuffer, *reasonBuffer;

   NEW(result, struct _Entry);    /*@@ possible memory leak */

   get(gui -> geDate, MUIA_String_Contents, & dateBuffer);
   result -> date = *myStrToDate(dateBuffer);
   get(gui -> geTransaction, MUIA_String_Contents, & transactionBuffer);
   get(gui -> geAmount, MUIA_String_Contents, & amountBuffer);
   get(gui -> geCheckNumber, MUIA_String_Contents, & checkNumberBuffer);
   get(gui -> geImputation, MUIA_String_Contents, & imputationBuffer);
   get(gui -> geReason, MUIA_String_Contents, & reasonBuffer);

   memcpy(result -> transaction, transactionBuffer, TRANSACTIONL);
   memcpy(result -> amount, amountBuffer, AMOUNTL);
   memcpy(result -> checkNumber, checkNumberBuffer, CHECKNUMBERL);
   memcpy(result -> imputation, imputationBuffer, IMPUTATIONL);
   memcpy(result -> reason, reasonBuffer, REASONL);

   result -> validated = FALSE;

   return result;
}

void
clearGetentryGadgets(Gui_t gui)
{
   setGetentryGadgetsToEntry(gui, NULL);
   if (CanReplace == TRUE)
     set (gui -> geReplace, MUIA_Disabled, FALSE);
   else
     set (gui -> geReplace, MUIA_Disabled, TRUE);
}

void
editEntry(Gui_t gui, Entry entry)
{
   if (! entry) {
      CanReplace = FALSE;
      set(gui -> geReplace, MUIA_Disabled, TRUE);
   }
   else {
      setGetentryGadgetsToEntry(gui, entry);
      LastSelectedEntry = *entry;
      CanReplace = TRUE;
      set(gui -> geReplace, MUIA_Disabled, FALSE);
   }
}

void
addEntry(Gui_t gui, Entries entries)
{
   Entry entry;

   entry = getEntryFromGadgets(gui);
   addEntryDatabase(entries, entry, 0, 0);   /* not a fake entry */
   FileModified = 1;
   fullDisplayList(gui, entries);
   clearGetentryGadgets(gui);
}

void
replaceEntry(Gui_t gui, Entries entries)
{
   Entry entry;

   entry = getEntryFromGadgets(gui);
   entry -> generatedBy = 0;
   entry -> fake = 0;
   DB_ReplaceEntry((DataBase) entries -> list, & LastSelectedEntry, entry);
   FileModified = 1;
   CanReplace = FALSE;
   fullDisplayList(gui, entries);
   clearGetentryGadgets(gui);
}
