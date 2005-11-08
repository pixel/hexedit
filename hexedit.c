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


optionParams options[LAST] = {
  { 8, 16, 256, "-s", "--sector" },
  { 4, 0, 0, "-m", "--maximize" },
  { 0, 0, 0, "-h", "--help" },
};
optionType option = maximized;


/*******************************************************************************/
/* main */
/*******************************************************************************/
int main(int argc, char **argv)
{
  int i, recognized;
  progName = basename(argv[0]);
  argv++; argc--;

  for (; argc > 0; argv++, argc--) 
    {
      recognized = FALSE;
      for (i = 0; !recognized && i < LAST; i++) {
	if (streq(*argv, options[i].shortOptionName) || 
	    streq(*argv, options[i].longOptionName)) {
	  if (i == helpOption) DIE(usage);
	  option = i;
	  recognized = TRUE;
	}
      }
      if (!recognized) break;
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





