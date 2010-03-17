/*
 * list.c
 */

/*  $Id: list.c,v 1.7 1993/09/03 00:17:28 beust Exp beust $  */

#include "common.h"


void
openListWindow(Gui_t gui, Entries entries)
{
   set(gui -> listWindowObject, MUIA_Window_Open, TRUE);
   fullDisplayList(gui, entries);
}

void
simpleDisplayList(Gui_t gui, Entries entries)
{
   DoMethod(gui -> liListObject, MUIM_List_Redraw, MUIV_List_Redraw_All);
   set(gui -> liListObject, MUIA_List_Active, MUIV_List_Active_Top);
}

void
fullDisplayList(Gui_t gui, Entries entries)
{
   char validatedString[40], totalString[40];
   int windowOpen;

  get(gui -> listWindowObject, MUIA_Window_Open, & windowOpen);
  if (windowOpen) {
  /* Create the new list */
     buildList(gui, entries, TRUE, TRUE, & gui -> entrySA);

  /* calculate the new totals */
     sprintf(validatedString, getString(MSG_VALIDATED),
             computeValidatedTotal(entries));
     set(gui -> liValidatedObject, MUIA_FrameTitle, validatedString);

     sprintf(totalString, getString(MSG_TOTAL), computeTotal(entries));
     set(gui -> liTotalObject, MUIA_FrameTitle, validatedString);

  /* Redraw the list */
     simpleDisplayList(gui, entries);
  }
}

void
handleValidate(Gui_t gui, Entries entries, Entry entry)
{
/* don't edit it if it was automatically generated */
   if (! entry -> fake) {

      entry -> validated = (entry -> validated == TRUE ? FALSE : TRUE);
      set(gui -> liListObject, MUIA_List_Quiet,FALSE);
      DoMethod(gui -> liListObject, MUIM_List_Redraw,MUIV_List_Redraw_Active);
      FileModified = 1;
   }
}

void
handleEdit(Gui_t gui, Entries entries, Entry entry)
{
   char validatedString[40], totalString[40];

/* edit it as an entry if it is a real one, or an automatic if it is fake */
   if (entry -> fake)
      editAutomatic(gui, entry -> generatedBy);
   else
      editEntry(gui, entry);
}

void
handleDelete(Gui_t gui, Entries entries, Entry entry)
{
   char validatedString[40], totalString[40];
   DataBase db = (DataBase) entries -> list;

   if (entry -> fake) {   /* warn about deleting an auto entry */
      if (showRequester(gui, getString(MSG_TITLE_DELETION),
                                  getString(MSG_BODY_DELETION_AUTO),
                                  getString(MSG_YES_OR_NO)) == 0) return;

      /* find the corresponding auto and delete it */
      db = (DataBase) entries -> automatic;
      deleteAutomatic(gui, entries, entry -> generatedBy);
   }
   else {
      if (showRequester(gui, getString(MSG_TITLE_DELETION),
                                  getString(MSG_BODY_DELETION_ENTRY),
                                  getString(MSG_YES_OR_NO)) == 0) return;

      /* Remove the selected entry */
      DoMethod(gui -> liListObject, MUIM_List_Remove, MUIV_List_Remove_Active);

      /* delete this entry */
      DB_RemoveEntry(db, entry);
      
   }

/* re-display the list */
   fullDisplayList(gui, entries);

/* Remember the file was modified */
   FileModified = 1;
}
