/*
 * gui.c
 */

/* $Id: gui.c,v 1.11 1993/09/03 00:16:55 beust Exp beust $ */

#include "common.h"

struct Library *MUIMasterBase;
struct _Gui Gui;

/* Id's returned when certain gadgets are pressed */

#define ID_GE_REPLACE 10
#define ID_GE_OK      11
#define ID_GE_CLEAR   12

#define ID_AU_NEW              20
#define ID_AU_ADD              21
#define ID_AU_REPLACE          22
#define ID_AU_DELETE           23
#define ID_AU_DOUBLECLICK      24

#define ID_LI_DOUBLECLICK 30

#define ID_MA_LIST 40
#define ID_MA_AUTO 41
#define ID_MA_GETENTRY 42

#define ID_MENU_START                         100

#define ID_MENU_FILE          (ID_MENU_START + 0)
#define ID_MENU_OPEN          (ID_MENU_FILE + 1)
#define ID_MENU_SAVE          (ID_MENU_OPEN + 1)
#define ID_MENU_SAVEAS        (ID_MENU_SAVE + 1)
#define ID_MENU_PRINT         (ID_MENU_SAVEAS + 1)
#define ID_MENU_EXPORT        (ID_MENU_PRINT + 1)
#define ID_MENU_QUIT          (ID_MENU_EXPORT + 1)

#define ID_MENU_PREFS         (ID_MENU_QUIT + 1)
#define ID_MENU_SAVEPREFS     (ID_MENU_PREFS + 1)

#define ID_MENU_END           ID_MENU_START + 50

/*************************/
/* Init & Fail Functions */
/*************************/

static void fail(APTR app,char *str)
{
   if (app)
      MUI_DisposeObject(app);

#ifndef _DCC
   if (MUIMasterBase)
      CloseLibrary(MUIMasterBase);
#endif

   if (str)
   {
      fprintf(stderr, "MUI error %d: %s\n", MUI_Error(), str);
      exit(20);
   }
}

static void init(void)
{
#ifndef _DCC
   if (!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
      fail(NULL,"Failed to open "MUIMASTER_NAME".");
#endif
}

#ifndef __SASC
static void stccpy(char *dest,char *source,int len)
{
   strncpy(dest,source,len);
   dest[len-1]='\0';
}

int wbmain(struct WBStartup *wb_startup)
{
   extern int main(int argc, char *argv[]);
   return (main(0, NULL));
}
#endif


/*****************
 * Private
 *****************/

static char *
returnListFormatString(Entries entries, struct ListEntry *le, char *result)
{
   int i;
   char acc[64];

   strcpy(result, "COL=0");    /* that's for '*' or ' ' */
   for (i=1; i < le -> numberOfColumns + 1; i++) {
      strcpy(acc, "");
      if (i != 0) strcat(result, ",");
      sprintf(acc, "COL=%d", i);
      strcat(result, acc);
   }
   return result;
}

APTR __saveds __asm
displayEntryFunc(register __a0 struct Hook *hook,
		 register __a2 char **array,
		 register __a1 void *entry)
{
   int i, j;
   LONG l[MAX_FIELDS + 1];
   char s[64];
   Entry ent = (Entry) entry;    /* works with auto and entry as well */
   struct ListEntry *le = (struct ListEntry *) hook -> h_Data;

   if (ent -> generatedBy) *array++ = "+ ";
   else if (ent -> validated) *array++ = "  ";
   else *array++ = "* ";
   for (i=0; i < le -> numberOfColumns; i++) {
      struct ColumnListEntry *col = & le -> col[i];
      for (j = 0; j < col -> numberOfFields; j++) {
         enum Fields f = (enum Fields) col -> fields[j];
         
         switch (f) {
            case DATEFIELD : l[j] = (LONG) myDateToStr(& ent -> date); break;
            case TRANSACTIONFIELD : l[j] = (LONG) ent -> transaction; break;
            case CHECKNUMBERFIELD : l[j] = (LONG) ent -> checkNumber; break;
            case AMOUNTFIELD : l[j] = (LONG) ent -> amount; break;
            case IMPUTATIONFIELD : l[j] = (LONG) ent -> imputation; break;
            case REASONFIELD : l[j] = (LONG) ent -> reason; break;
         }
      }
      l[j] = NULL;
      mySprintf(s, col -> format, l);
      *array++ = strdup(s);
   }
   return NULL;
}

struct Hook displayEntryHook = {
        {NULL, NULL},
        (void *)displayEntryFunc,
        NULL, & DisplayEntryFormat
};

APTR __saveds __asm
displayAutomaticFunc(register __a0 struct Hook *hook,
		     register __a2 char **array,
		     register __a1 void *entry)
{
   Automatic ent = (Automatic) entry;    /* works with auto and entry as well */
   *array = ent -> formattedString;
   return NULL;
}

struct Hook displayAutomaticHook = {
        {NULL, NULL},
        (void *)displayAutomaticFunc,
        NULL, NULL
};

LONG __saveds __asm
compareEntryFunc(register __a0 struct Hook *hook,
		 register __a1 Entry p1,
		 register __a2 Entry p2)
/* Actually, compare in reverse order, so that the most recent date */
/* appears on top */
{
  
   LONG result = ((LONG) p2 -> date.ds_Days) - ((LONG) p1 -> date.ds_Days);
/*
   if (count++ < 30) {
      D(printf("'%s' (%d)'%s' (%d) -> %d\n",
	       myDateToStr(&p1->date), p1->date.ds_Days,
	       myDateToStr(&p2->date), p2->date.ds_Days,
	      result));
   }
*/
   return result;
}

static struct Hook compareEntryHook = {
	{NULL, NULL},
	(void *) compareEntryFunc,
	NULL, NULL
};

/************
 * Menu related functions
 ************/

/* The strings are actually filled by the function initMenus() */
void
initMenus(Gui_t gui, struct NewMenu *nm)
/* Set up the menus */
{
   int i = 0;

   addToNewMenu(nm, gui, NM_TITLE, getString(MSG_MENU_FILE), NULL,
                0, NULL, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_OPEN), "o",
                   ID_MENU_OPEN, menuOpen, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_SAVE), "w",
                   ID_MENU_SAVE, menuSave, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_SAVE_AS), "a",
                   ID_MENU_SAVEAS, menuSaveAs, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_PRINT), "p",
                   ID_MENU_PRINT, menuPrint, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_EXPORT), "e",
                   ID_MENU_EXPORT, menuExport, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_QUIT), "q",
                   MUIV_Application_ReturnID_Quit, NULL, i++);
   addToNewMenu(nm, gui, NM_TITLE, getString(MSG_MENU_PREFERENCES), NULL,
                0, NULL, i++);
      addToNewMenu(nm, gui, NM_ITEM, getString(MSG_MENU_FILE_SAVE), NULL,
                   ID_MENU_SAVEPREFS, menuSavePrefs, i++);


   addToNewMenu(nm, gui, NM_END, NULL, NULL,
                0, NULL, i++);

   if (i >= MENU_ITEM_COUNT) {
      fprintf(stderr, "too many menus!\n");
   }
}

/**************
 * Main function
 **************/


int
buildGui(Gui_t gui, Entries entries, struct OptionsFileStruct *ofStruct)
{
   ULONG signal;
   APTR app;
   APTR mainWindowObject, newEntryWin;
   APTR pb1, pb2, pb3, pb4;        /*  main window objects */
   APTR s1, s2, s3, s4, s5, s6, c1, pbok, pbrepl, pbclear; /* get entry objects */
   APTR s10, s11, s12, s13, s14, pb10, pb11, pb12, pb13,
        s15, s16, s17, s18, lv10;   /* getauto objects */
   APTR lv20, cy20, te20, te21;   /* list objects */
   char *payment[10], *actions[10];
   char validatedString[40], totalString[40];
   float total;
   Entry entry;
   int i, running, cycle;
   Entry activeEntry;
   Automatic activeAutomatic;
   struct NewMenu *mainMenu;
   char listFormatString[128];   /* will contain "COL=0,COL=1,..." */

/* Fill the constant listviews */
   i = 0;
   payment[i++] = getString(MSG_CHECK);
   payment[i++] = getString(MSG_CREDITCARD);
   payment[i++] = NULL;

   i = 0;
   actions[i++] = getString(MSG_ACCOUNT);
   actions[i++] = getString(MSG_EDIT);
   actions[i++] = getString(MSG_DELETE);
   actions[i++] = NULL;

   sprintf(validatedString, getString(MSG_VALIDATED),
           computeValidatedTotal(entries));
   total = computeTotal(entries);
   sprintf(totalString, getString(MSG_TOTAL), total);

/* Initialize MUI */
   init();

/* Initialize the GUI's global area */
   memset(& gui -> autoSA, 0, sizeof(gui -> autoSA));  /* smartalloc structures */
   memset(& gui -> entrySA, 0, sizeof(gui -> entrySA));
   mainMenu = (struct NewMenu *) malloc(sizeof(struct NewMenu) * MENU_ITEM_COUNT);
   gui -> menuFunctions = (void **) malloc(sizeof(void **) * MENU_ITEM_COUNT);
   initMenus(gui, mainMenu);
   returnListFormatString(entries, DisplayEntryFormat, listFormatString);

   /*
   ** Create the application.
   ** Note that we generate two empty groups without children.
   ** These children will be added later with OM_ADDMEMBER.
   */

   gui -> app = app = ApplicationObject,
      MUIA_Application_Title      , "Banker",
      MUIA_Application_Version    , "$VER: 2.0beta (12.93)",
      MUIA_Application_Copyright  , "©1992/93, Cédric BEUST",
      MUIA_Application_Author     , "Cédric BEUST",
      MUIA_Application_Description, "A home budget program",
      MUIA_Application_Base       , "BANKER",
      MUIA_Application_Menu       , mainMenu,

/*
 * MainWindow
 */

      SubWindow, gui -> mainWindowObject = mainWindowObject = WindowObject,
            MUIA_Window_ID, ID_MAINWINDOW,
            MUIA_Window_Title, "Banker",
            WindowContents,  VGroup,
               Child, pb1 = KeyButton(getString(MSG_WINDOW_NEW_ENTRY_WITH__),
                           getSignificantCharacter(MSG_WINDOW_NEW_ENTRY_WITH__)),
               Child, pb2 = KeyButton(getString(MSG_WINDOW_LIST_ENTRIES),
                           getSignificantCharacter(MSG_WINDOW_LIST_ENTRIES)),
               Child, pb3 = KeyButton(getString(MSG_WINDOW_PERIODIC_ENTRY),
                           getSignificantCharacter(MSG_WINDOW_PERIODIC_ENTRY)),
               Child, pb4 = KeyButton(getString(MSG_WINDOW_CLEAR_FILE),
                           getSignificantCharacter(MSG_WINDOW_CLEAR_FILE)),
             End,  /* windowcontents */
      End,  /* subwindow */

/*
 * GetEntry Window
 */

      SubWindow, gui -> getEntryWindowObject = newEntryWin = WindowObject,
          MUIA_Window_ID, ID_GETENTRYWINDOW,
          MUIA_Window_Title, getString(MSG_WINDOW_NEW_ENTRY),
             WindowContents, VGroup,
             Child, HGroup,
                   Child, gui -> geDate = s1 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, DATEL,
                      MUIA_FrameTitle, getString(MSG_GADGET_DATE),
                   End,
                   Child, gui -> geCheckNumber = s2 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, CHECKNUMBERL,
                      MUIA_FrameTitle, getString(MSG_GADGET_CHECKNUMBER),
                   End,
             End,
             Child, HGroup,
                   Child, gui -> geTransaction = s3 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, TRANSACTIONL,
                      MUIA_FrameTitle, getString(MSG_GADGET_TRANSACTION),
                   End,
                   Child, gui -> geImputation = s4 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, IMPUTATIONL,
                      MUIA_FrameTitle, getString(MSG_GADGET_IMPUTATION),
                   End,
             End,
             Child, HGroup,
                   Child, gui -> geAmount = s5 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, AMOUNTL,
                      MUIA_FrameTitle, getString(MSG_GADGET_MONTANT),
                   End,
                   Child, gui -> geReason = s6 = StringObject,
                      StringFrame,
                      MUIA_String_MaxLen, REASONL,
                      MUIA_FrameTitle, getString(MSG_GADGET_LIBELLE),
                   End,
             End,
             Child, HGroup,
                Child, RectangleObject,
                   MUIA_Weight, 300,
                End,
                Child, c1 = CycleObject,
                   MUIA_Cycle_Entries, payment,
                End,
                Child, RectangleObject,
                   MUIA_Weight, 300,
                End,
             End,
             Child, HGroup,
                Child, RectangleObject,
                   MUIA_Weight, 150,
                End,
                Child, pbok = KeyButton(getString(MSG_OK),
                                 getSignificantCharacter(MSG_OK)),
                Child, RectangleObject,
                   MUIA_Weight, 150,
                End,
                Child, gui->geReplace=pbrepl=KeyButton(getString(MSG_REPLACE_FULL),
                                 getSignificantCharacter(MSG_REPLACE_FULL)),
                Child, RectangleObject,
                   MUIA_Weight, 150,
                End,
                Child, pbclear = KeyButton(getString(MSG_CLEAR),
                                 getSignificantCharacter(MSG_CLEAR)),
                Child, RectangleObject,
                   MUIA_Weight, 150,
                End,
                MUIA_Weight, 0,
             End,
             
          End, /* vgroup */
      End,  /* subwindow */

/*
 * List Window
 */

      SubWindow, gui -> listWindowObject = WindowObject,
          MUIA_Window_ID, ID_LISTWINDOW,
          MUIA_Window_Title, "List",
             WindowContents, VGroup,
                Child, gui -> liListviewObject = lv20 = ListviewObject,
                   MUIA_Listview_Input, TRUE,
                   MUIA_Listview_List, gui -> liListObject = ListObject,
                      ReadListFrame,
                      MUIA_List_DisplayHook, & displayEntryHook,
                      MUIA_List_CompareHook, & compareEntryHook,
                      MUIA_List_Format, listFormatString,
                   End,  /* listobject */
                End,  /* list */
                Child, HGroup,
                   Child, RectangleObject,
                      MUIA_Weight, 100,
                   End,
                   Child, gui -> liCycleObject = cy20 = CycleObject,
                      MUIA_Cycle_Entries, actions,
                   End, /* cycle */
                   Child, RectangleObject,
                      MUIA_Weight, 50,
                   End,
                   Child, VGroup,
                      GroupFrame,
                      Child, gui -> liValidatedObject = te20 = RectangleObject,
                         MUIA_FrameTitle, validatedString,
                      End,
                      Child, gui -> liTotalObject = te21 = RectangleObject,
                         MUIA_FrameTitle, totalString,
                      End,
                   End, /* group */
                   Child, RectangleObject,
                      MUIA_Weight, 30,
                   End,
                End,  /* group */
          End, /* hgroup */
      End, /* subwindow */

/*
 * GetAutomatic Window
 */

      SubWindow, gui -> automaticWindowObject = WindowObject,
          MUIA_Window_ID, ID_AUTOMATICWINDOW,
          MUIA_Window_Title, getString(MSG_WINDOW_AUTOMATIC_ENTRIES),
             WindowContents, HGroup,
                Child, VGroup,
                   GroupFrame,
                   Child, HGroup,
                      Child, gui -> auFrom = s10 = StringObject,
                         StringFrame,
                         MUIA_String_MaxLen, 30,
                         MUIA_FrameTitle, getString(MSG_START_AT_DATE),
                      End, /* object */
                      Child, gui -> auTo = s11 = StringObject,
                         StringFrame,
                         MUIA_String_MaxLen, 30,
                         MUIA_FrameTitle, getString(MSG_END_AT_DATE),
                      End, /* object */
                      Child, gui -> auAmount = s12 = StringObject,
                         StringFrame,
                         MUIA_String_MaxLen, AMOUNTL,
                         MUIA_FrameTitle, getString(MSG_GADGET_MONTANT),
                      End, /* object */
                   End, /* hgroup */

/*
                   Child, RectangleObject,
                      MUIA_Weight, 200,
                   End,
*/

                   Child, HGroup,
                      Child, VGroup,
                         Child, gui -> auDays = s13 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, 10,
                            MUIA_FrameTitle, getString(MSG_DAYS),
                            MUIA_Weight, 50,
                         End, /* object */
                         Child, gui -> auWeeks = s14 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, 10,
                            MUIA_FrameTitle, getString(MSG_WEEKS),
                            MUIA_Weight, 50,
                         End, /* object */
                         Child, gui -> auMonths = s15 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, 10,
                            MUIA_FrameTitle, getString(MSG_MONTHS),
                            MUIA_Weight, 50,
                         End, /* object */
                         Child, gui -> auYears = s16 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, 10,
                            MUIA_FrameTitle, getString(MSG_YEARS),
                            MUIA_Weight, 50,
                         End, /* object */
                      End, /* vgroup */
                      Child, RectangleObject,
                         MUIA_Weight, 50,
                      End,
                      Child, VGroup,
                         Child, RectangleObject,
                            MUIA_Weight, 50,
                         End,
                         Child, gui->auNew=pb10= KeyButton(getString(MSG_NEW_FULL),
                                           getSignificantCharacter(MSG_NEW_FULL)),
                         Child, RectangleObject,
                            MUIA_Weight, 50,
                         End,
                         Child, gui->auAdd=pb11= KeyButton(getString(MSG_ADD_FULL),
                                           getSignificantCharacter(MSG_ADD_FULL)),
                         Child, RectangleObject,
                            MUIA_Weight, 50,
                         End,
                         Child, gui->auReplace=pb12 = KeyButton(getString(MSG_REPLACE_FULL),
                                      getSignificantCharacter(MSG_REPLACE_FULL)),
                         Child, RectangleObject,
                            MUIA_Weight, 50,
                         End,
                         Child, gui->auDelete=pb13 = KeyButton(getString(MSG_DELETE_FULL),
                                        getSignificantCharacter(MSG_DELETE_FULL)),
                      End, /* vgroup */
                   End, /* hgroup */

/*
                   Child, RectangleObject,
                      MUIA_Weight, 200,
                   End,
*/

                   Child, HGroup,
                         Child, gui -> auReason = s17 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, REASONL,
                            MUIA_FrameTitle,getString(MSG_GADGET_TRANSACTION),
                         End, /* object */
                   End, /* hgroup */
                   Child, HGroup,
                         Child, gui -> auImputation = s18 = StringObject,
                             StringFrame,
                            MUIA_String_MaxLen, IMPUTATIONL,
                            MUIA_FrameTitle, getString(MSG_GADGET_IMPUTATION),
                         End, /* object */
                   End, /* hgroup */
                End, /* vgroup */

                Child, gui -> auListviewObject = lv10 = ListviewObject,
                   MUIA_Listview_Input, TRUE,
                   MUIA_Listview_List, gui -> auListObject = ListObject,
                      MUIA_List_DisplayHook, & displayAutomaticHook,
                      MUIA_List_Format, "DELTA=8",
                      ReadListFrame,
                   End,  /* listobject */
                End,  /* listviewobject */

         End, /* hgroup */
      End, /* windowcontents */
   End;  /* app */

   if (! app) fail(app,"Failed to create Application.");
   if (! gui -> mainWindowObject) fail(app, "Couldn't open main window");


   /*
   ** Main window settings
   */

   DoMethod(gui -> mainWindowObject,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,
            app,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);


   /* Connect the buttons to open the proper windows */
   DoMethod(pb1, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_MA_GETENTRY);
   DoMethod(pb2, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_MA_LIST);
   DoMethod(pb3, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_MA_AUTO);

   /*
   ** GetEntry window settings
   */

   DoMethod(pbok, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_GE_OK);
   DoMethod(pbrepl, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_GE_REPLACE);
   DoMethod(pbclear, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_GE_CLEAR);
   DoMethod(gui->getEntryWindowObject, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            gui -> getEntryWindowObject, 3, MUIM_Set, MUIA_Window_Open, FALSE);
   DoMethod(gui -> getEntryWindowObject, MUIM_Window_SetCycleChain,
            s1, s2, s3, s4, s5, s6, c1, pbok, pbrepl, pbclear, NULL);


   /*
   ** List window settings
   */

   DoMethod(gui-> listWindowObject,MUIM_Notify,MUIA_Window_CloseRequest, TRUE,
            gui -> listWindowObject, 3, MUIM_Set, MUIA_Window_Open, FALSE);
   DoMethod(gui-> liListviewObject, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            app, 2, MUIM_Application_ReturnID, ID_LI_DOUBLECLICK);
   DoMethod(gui -> listWindowObject, MUIM_Window_SetCycleChain,
            lv20, cy20, NULL);


   /*
   ** Automatic window settings
   */

   DoMethod(gui->automaticWindowObject,MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            gui -> automaticWindowObject, 3, MUIM_Set, MUIA_Window_Open, FALSE);
   DoMethod(gui ->automaticWindowObject, MUIM_Window_SetCycleChain,
            s10, s11, s12, s13, s14, s15, s16, s17, s18, 
            pb10, pb11, pb12, pb13, lv10, NULL);
   DoMethod(pb10, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_AU_NEW);
   DoMethod(pb11, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_AU_ADD);
   DoMethod(pb12, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_AU_REPLACE);
   DoMethod(pb13, MUIM_Notify, MUIA_Pressed, FALSE,
            app, 2, MUIM_Application_ReturnID, ID_AU_DELETE);
   DoMethod(gui-> auListviewObject, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
            app, 2, MUIM_Application_ReturnID, ID_AU_DOUBLECLICK);
   set(pb12, MUIA_Disabled, TRUE);
   set(pb13, MUIA_Disabled, TRUE);

   /*
   ** Open the windows
   */

   set(gui -> mainWindowObject, MUIA_Window_Open, TRUE);
   if (ofStruct -> getEntry.isOpen) openGetentryWindow(gui, entries);
   if (ofStruct -> automatic.isOpen) openAutomaticWindow(gui, entries);
   if (ofStruct -> list.isOpen) openListWindow(gui, entries);

   running = 1;
   while (running) {
     int id;
     id = DoMethod(app,MUIM_Application_Input,&signal);

     /*
     ** If it's a menu, I have an array of functions for each of then
     */
     if (id >= ID_MENU_START && id <= ID_MENU_END) {
        void (*f)();
        f = (void (*)()) gui -> menuFunctions[id - ID_MENU_START];
        if (f) (*f)(gui, entries);
     }

     switch(id) {
       case MUIV_Application_ReturnID_Quit :
         if (FileModified) {
            if (showRequester(gui, getString(MSG_TITLE_FILE_MODIFIED),
		   	        getString(MSG_FILE_MODIFIED_QUIT),
			           getString(MSG_YES_OR_NO)) == 0) break;
            else
               running = 0;
         }
         break;

    /*
    ** These cases are related to the Main window
    */

       case ID_MA_LIST :
         openListWindow(gui, entries);
         break;

    /*
    ** These cases are related to the List window
    */

       case ID_LI_DOUBLECLICK :
         DoMethod(gui -> liListviewObject, MUIM_List_GetEntry,
                  MUIV_List_GetEntry_Active, & activeEntry);
         get(gui -> liCycleObject, MUIA_Cycle_Active, & cycle);
         switch(cycle) {
            case 0 :
              handleValidate(gui, entries, activeEntry); break;
            case 1 :
              handleEdit(gui, entries, activeEntry); break;
            case 2 :
              handleDelete(gui, entries, activeEntry); break;
            default :;
         }
         break;

    /*
    ** These cases are related to the GetEntry window
    */

       case ID_MA_GETENTRY :
         openGetentryWindow(gui, entries);
         break;
       case ID_GE_OK :
         addEntry(gui, entries);
         break;
       case ID_GE_REPLACE :
         replaceEntry(gui, entries);
         break;
       case ID_GE_CLEAR :
         clearGetentryGadgets(gui);
         break;

    /*
    ** These cases are related to the Automatic window
    */

       case ID_MA_AUTO :
         openAutomaticWindow(gui, entries);
         break;
       case ID_AU_NEW :
         newAutomatic(gui, entries);
         break;
       case ID_AU_REPLACE :
         replaceAutomatic(gui, entries);
         break;
       case ID_AU_ADD :
         addAutomatic(gui, entries);
         break;
       case ID_AU_DELETE :
         deleteAutomatic(gui, entries, NULL);
         break;
       case ID_AU_DOUBLECLICK :
         DoMethod(gui -> auListviewObject, MUIM_List_GetEntry,
                  MUIV_List_GetEntry_Active, & activeAutomatic);
         editAutomatic(gui, activeAutomatic);
     }

     if (signal) Wait(signal);
   }

   set(gui -> mainWindowObject,MUIA_Window_Open,FALSE);

   fail(app,NULL);
   return 0;
}
