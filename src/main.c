/*
 * main.c
 */

/* $Id: main.c,v 1.2 1993/09/03 00:17:28 beust Exp beust $ */

/*
 * A very simple module, always compiled with full symbols to keep the
 * debugger happy
 */

#include "common.h"

/* Version */
#define BANKERVERSION "2"
#define BANKERREVISION "0Beta"
#define BANKERDATE "Aug-93"

static const char Version[]="\0$VER: Banker " BANKERVERSION "." BANKERREVISION
                            " (" BANKERDATE ")\r\n";

int
main(int argc, char **argv)
{
   char *s, *source;
   FILE *logFile;
   struct DateStamp *date;
   int i;

   openLibraries(& MainList);
   readEnvironment(argc, argv);

/* Open a file if requested */
   if (CL_Struct.filename) {   /* a filename was specified on the command line */
      openFile(& Gui, MainList, CL_Struct.filename);
   }

   buildGui(& Gui, MainList, & OF_Struct);
/*
   openLocale(CL_Struct.language);
*/
#ifdef A
   setAlarm();
   initGadtools();
   mainWindow(MainList);
   closeAll();
#endif
   return 0;
}

