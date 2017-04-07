/* hexedit -- Hexadecimal Editor for Binary Files
   Copyright (C) 1998 Pixel (Pascal Rigaux)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.*/
#include "hexedit.h"


/*******************************************************************************/
/* Global variables */
/*******************************************************************************/
INT lastEditedLoc, biggestLoc, fileSize;
INT mark_min, mark_max, mark_set;
INT base, oldbase;
int normalSpaces, cursor, cursorOffset, hexOrAscii;
int cursor, blocSize, lineLength, colsUsed, page;
int isReadOnly, fd, nbBytes, oldcursor, oldattr, oldcursorOffset;
int sizeCopyBuffer, *bufferAttr;
char *progName, *fileName, *baseName;
unsigned char *buffer, *copyBuffer;
typePage *edited;

char *lastFindFile = NULL, *lastYankToAFile = NULL, *lastAskHexString = NULL, *lastAskAsciiString = NULL, *lastFillWithStringHexa = NULL, *lastFillWithStringAscii = NULL;


const modeParams modes[LAST] = {
  { 8, 16, 256 },
  { 4, 0, 0 },
};
modeType mode = maximized;
int colored = FALSE;

const char * const usage = "usage: %s [-s | --sector] [-m | --maximize] [-l<n> | --linelength <n>]"
#ifdef HAVE_COLORS 
     " [--color]"
#endif 
     " [-h | --help] filename\n";


/*******************************************************************************/
/* main */
/*******************************************************************************/
int main(int argc, char **argv)
{
  progName = basename(argv[0]);
  argv++; argc--;

  for (; argc > 0; argv++, argc--) 
    {
      if (streq(*argv, "-s") || streq(*argv, "--sector"))
	mode = bySector;
      else if (streq(*argv, "-m") || streq(*argv, "--maximize")) {
	mode = maximized;
	lineLength = 0;
      }
#ifdef HAVE_COLORS
      else if (streq(*argv, "--color"))
	colored = TRUE;
#endif
      else if (strbeginswith(*argv, "-l") || strbeginswith(*argv, "--linelength")) {
	if (strbeginswith(*argv, "-l") && strlen(*argv) > 2)
	  lineLength = atoi(*argv + 2);
	else {
	  argv++; argc--;
	  lineLength = atoi(*argv);
	}
      } else if (streq(*argv, "--")) {
	argv++; argc--;
	break;
      } else if (*argv[0] == '-')

	DIE(usage)
      else break;
    }
  if (argc > 1) DIE(usage);

  init();
  if (argc == 1) {
    fileName = strdup(*argv);
    openFile();
  }
  initCurses();
  if (fileName == NULL) {
    if (!findFile()) {
      exitCurses();
      DIE("%s: No such file\n");
    }
    openFile();
  }
  readFile();
  do display();
  while (key_to_function(getch()));
  quit();
  return 0; /* for no warning */
}



/*******************************************************************************/
/* other functions */
/*******************************************************************************/
void init(void)
{
  page = 0; /* page == 0 means initCurses is not done */
  normalSpaces = 3;
  hexOrAscii = TRUE;
  copyBuffer = NULL;
  edited = NULL;
}

void quit(void)
{
  exitCurses();
  free(fileName);
  free(buffer);
  free(bufferAttr);
  FREE(copyBuffer);
  discardEdited();
  FREE(lastFindFile); FREE(lastYankToAFile); FREE(lastAskHexString); FREE(lastAskAsciiString); FREE(lastFillWithStringHexa); FREE(lastFillWithStringAscii);
  exit(0);
}





