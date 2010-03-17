/*
 * automatic.c
 */

/* $Id: automatic.c,v 1.4 1993/09/03 00:17:28 beust Exp beust $ */

#include "common.h"
#include "locale_strings.h"

static struct _Automatic LastSelectedAutomatic;

/********************************************************************************
 * Private part
 ********************************************************************************/

static Automatic
getAutomaticFromGadgets(Gui_t gui)
{
   Automatic result;
   char *fromBuffer, *toBuffer, *amountBuffer, *daysBuffer, *weeksBuffer,
        *monthsBuffer, *yearsBuffer, *transactionBuffer, *imputationBuffer;

   NEW(result, struct _Automatic);

   get(gui -> auFrom, MUIA_String_Contents, & fromBuffer);
   get(gui -> auTo, MUIA_String_Contents, & toBuffer);
   get(gui -> auAmount, MUIA_String_Contents, & amountBuffer);
   get(gui -> auDays, MUIA_String_Contents, & daysBuffer);
   get(gui -> auWeeks, MUIA_String_Contents, & weeksBuffer);
   get(gui -> auMonths, MUIA_String_Contents, & monthsBuffer);
   get(gui -> auYears, MUIA_String_Contents, & yearsBuffer);
   get(gui -> auReason, MUIA_String_Contents, & transactionBuffer);
   get(gui -> auImputation, MUIA_String_Contents, & imputationBuffer);

   result -> first = *myStrToDate(fromBuffer);
   result -> last = *myStrToDate(toBuffer);
   result -> repeatDays = atoi(daysBuffer);
   result -> repeatWeeks = atoi(weeksBuffer);
   result -> repeatMonths = atoi(monthsBuffer);
   result -> repeatYears = atoi(yearsBuffer);
   strcpy(result -> transaction, transactionBuffer);
   strcpy(result -> imputation, imputationBuffer);
   strcpy(result -> amount, amountBuffer);

   return result;
}

static void
setAutomaticGadgetsToAuto(Gui_t gui, Automatic aut)
{
   struct DateStamp current;
   int windowOpen;

   get(gui -> automaticWindowObject, MUIA_Window_Open, & windowOpen);
   if (windowOpen) {
      set(gui -> automaticWindowObject, MUIA_Window_Open, TRUE);
      DateStamp(& current);
      if (aut == NULL) {
         set(gui -> auFrom, MUIA_String_Contents,  myDateToStr(& current));
         set(gui -> auTo, MUIA_String_Contents, "");
         set(gui -> auAmount, MUIA_String_Contents, "");
         set(gui -> auDays, MUIA_String_Contents, 0);
         set(gui -> auWeeks, MUIA_String_Contents, 0);
         set(gui -> auMonths, MUIA_String_Contents, 0);
         set(gui -> auYears, MUIA_String_Contents, 0);
         set(gui -> auReason, MUIA_String_Contents, "");
         set(gui -> auImputation, MUIA_String_Contents, "");
      }
      else {
	 char days[8], weeks[8], months[8], years[8];
	 sprintf(days, "%d", aut -> repeatDays);
	 sprintf(weeks, "%d", aut -> repeatWeeks);
	 sprintf(months, "%d", aut -> repeatMonths);
	 sprintf(years, "%d", aut -> repeatYears);
         set(gui -> auFrom, MUIA_String_Contents, myDateToStr(& aut -> first));
         set(gui -> auTo, MUIA_String_Contents, myDateToStr(& aut -> last));
         set(gui -> auAmount, MUIA_String_Contents, aut -> amount);
         set(gui -> auDays, MUIA_String_Contents, days);
         set(gui -> auWeeks, MUIA_String_Contents, weeks);
         set(gui -> auMonths, MUIA_String_Contents, months);
         set(gui -> auYears, MUIA_String_Contents, years);
         set(gui -> auReason, MUIA_String_Contents, aut -> transaction);
         set(gui -> auImputation, MUIA_String_Contents, aut -> imputation);
      }
   }
}

/********************************************************************************
 * Public part
 ********************************************************************************/


void
clearAutomaticGadgets(Gui_t gui)
{
   setAutomaticGadgetsToAuto(gui, NULL);
   set(gui -> auNew, MUIA_Disabled, FALSE);
   set(gui -> auAdd, MUIA_Disabled, FALSE);
   set(gui -> auReplace, MUIA_Disabled, TRUE);
   set(gui -> auDelete, MUIA_Disabled, TRUE);
}


void
openAutomaticWindow(Gui_t gui, Entries entries)
{
   struct MinList *ml;
   struct Node *node;

   /* Open the window */
   set(gui -> automaticWindowObject, MUIA_Window_Open, TRUE);

   /* Fill the gadgets */
   buildList(gui, entries, FALSE, TRUE, & gui -> autoSA);
                          /* an automatic, want it sorted */
   setAutomaticGadgetsToAuto(gui, NULL);

/*@@ running the list
   printf("auto list:\n");
   for (node = ml -> lh_Head; node -> ln_Succ; node = node -> ln_Succ) {
      printf("node: '%s'\n", node -> ln_Name);
   }
*/
}

void
newAutomatic(Gui_t gui, Entries entries)
{
   clearAutomaticGadgets(gui);
}

void
replaceAutomatic(Gui_t gui, Entries entries)
{
   Automatic new, old;
   new = getAutomaticFromGadgets(gui);
   old = & LastSelectedAutomatic;

   /* Replace the old entry */
   DB_ReplaceEntry((DataBase) entries -> automatic, old, new);

   buildList(gui, entries, FALSE, TRUE, & gui -> autoSA);
                          /* an automatic, want it sorted */
   fullDisplayList(gui, entries);
   FileModified = 1;
   clearAutomaticGadgets(gui);

}

void
addAutomatic(Gui_t gui, Entries entries)
{
   Automatic aut;
   aut = getAutomaticFromGadgets(gui);

   /* Add this entry */
   DB_AddEntry((DataBase) entries -> automatic, aut);

   FileModified = 1;

   /* update the main list window */
   buildList(gui, entries, FALSE, TRUE, & gui -> autoSA);
                          /* an automatic, want it sorted */

   fullDisplayList(gui, entries);

   clearAutomaticGadgets(gui);

}

void
deleteAutomatic(Gui_t gui, Entries entries, Automatic aut)
{
   int n;
   if (aut == NULL) {
      get(gui -> auListObject, MUIA_List_Active, &n);
      aut = DB_NthEntry(entries -> automatic, n);
   }

   /* Delete this entry */
   DB_RemoveEntry((DataBase) entries -> automatic, aut);
   FileModified = 1;
      
   buildList(gui, entries, FALSE, TRUE, & gui -> autoSA);
   fullDisplayList(gui, entries);
   clearAutomaticGadgets(gui);
}

void
editAutomatic(Gui_t gui, Automatic automatic)
{
   set(gui -> auNew, MUIA_Disabled, FALSE);
   set(gui -> auAdd, MUIA_Disabled, FALSE);

   setAutomaticGadgetsToAuto(gui, automatic);
   if (automatic) {
      LastSelectedAutomatic = *automatic;
      set(gui -> auReplace, MUIA_Disabled, FALSE);
      set(gui -> auDelete, MUIA_Disabled, FALSE);
   }
   else {
      set(gui -> auReplace, MUIA_Disabled, TRUE);
      set(gui -> auDelete, MUIA_Disabled, TRUE);
   }
}

