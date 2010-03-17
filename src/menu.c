/*
 * menu.c
 */

/* $Id: menu.c,v 1.3 1993/09/03 00:17:28 beust Exp beust $ */

#include "common.h"

void
addToNewMenu(struct NewMenu *nm, Gui_t gui,
	     UBYTE type, STRPTR label, STRPTR shortCut,
	     int id, APTR callback, int n)
{
   nm[n].nm_Type = type;
   nm[n].nm_Label = label;
   nm[n].nm_CommKey = shortCut;
   nm[n].nm_Flags = 0;
   nm[n].nm_MutualExclude = 0;
   nm[n].nm_UserData = (void *) id;
   gui -> menuFunctions[n] = callback;
}

MENU_CALLBACK(menuOpen)
{
   if (FileModified) {
      if (showRequester(gui, getString(MSG_TITLE_FILE_MODIFIED),
			     getString(MSG_BODY_FILE_MODIFIED),
			     getString(MSG_YES_OR_NO)) == 0) return;
   }
   openFile(gui, entries, NULL);
   set(gui -> mainWindowObject, MUIA_Window_Title, FileName);
}

MENU_CALLBACK(menuSave)
{
   if (STREQUAL(FileName, getString(MSG_NO_FILE_LOADED)))
      getFileName(gui, FileName, getString(MSG_MENU_FILE_SAVE_AS), FileName, Pattern);

   saveFile(entries, FileName);
   FileModified = 0;
}

MENU_CALLBACK(menuSaveAs)
{
   getFileName(gui, FileName, getString(MSG_MENU_FILE_SAVE_AS), FileName, Pattern);
   menuSave(gui, entries);
   FileModified = 0;
}

MENU_CALLBACK(menuPrint)
{
   getFileName(gui, PrintFileName, getString(MSG_MENU_FILE_PRINT),
               PrintFileName, PrintPattern);

   writeTextFile(entries, PrintFileName, & PrintFormatStructure);
}

MENU_CALLBACK(menuExport)
{
   getFileName(gui, ExportFileName, getString(MSG_MENU_FILE_EXPORT),
               ExportFileName, ExportPattern);

   writeTextFile(entries, ExportFileName, & ExportFormatStructure);
}

MENU_CALLBACK(menuSavePrefs)
{
   writeOptionsFile(gui);
}
