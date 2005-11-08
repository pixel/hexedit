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

static int searchA(char **string, int *sizea, char *tmp, int tmp_size);
static void searchB(INT loc, char *string);

/*******************************************************************************/
/* Search functions */
/*******************************************************************************/
static int searchA(char **string, int *sizea, char *tmp, int tmp_size)
{
  char *msg = hexOrAscii ? "Hexa string to search: " : "Ascii string to search: ";
  char **last = hexOrAscii ? &lastAskHexString : &lastAskAsciiString;

  if (!ask_about_save_and_redisplay()) return FALSE;
  if (!displayMessageAndGetString(msg, last, tmp, tmp_size)) return FALSE;

  *sizea = strlen(tmp);
  if (hexOrAscii) if (!hexStringToBinString(tmp, sizea)) return FALSE;

  *string = malloc(*sizea);
  memcpy(*string, tmp, *sizea);

  nodelay(stdscr, TRUE);
  displayTwoLineMessage("searching...", "(press any key to cancel)");
  return TRUE;
}

static void searchB(INT loc, char *string)
{
  nodelay(stdscr, FALSE);
  free(string);

  if (loc >= 0) set_cursor(loc);
  else {
    if (loc == -3) displayMessageAndWaitForKey("not found");
  }
}

void search_forward(void)
{
  char *p, *string, tmp[BLOCK_SEARCH_SIZE], tmpstr[BLOCK_SEARCH_SIZE];
  int quit, sizea, sizeb;
  INT blockstart;

  if (!searchA(&string, &sizea, tmp, sizeof(tmp))) return;
  quit = -1;
  blockstart = base + cursor - BLOCK_SEARCH_SIZE + sizea;
  do {
    blockstart += BLOCK_SEARCH_SIZE - sizea + 1;
    if (LSEEK_(fd, blockstart) == -1) { quit = -3; break; }
    if ((sizeb = read(fd, tmp, BLOCK_SEARCH_SIZE)) < sizea) quit = -3;
    else if (getch() != ERR) quit = -2;
    else if ((p = mymemmem(tmp, sizeb, string, sizea))) quit = p - tmp;

    sprintf(tmpstr,"searching... 0x%08llX", (long long) blockstart);
    nodelay(stdscr, TRUE);
    displayTwoLineMessage(tmpstr, "(press any key to cancel)");

  } while (quit == -1);

  searchB(quit + (quit >= 0 ? blockstart : 0), string);
}

void search_backward(void)
{
  char *p, *string, tmp[BLOCK_SEARCH_SIZE], tmpstr[BLOCK_SEARCH_SIZE];
  int quit, sizea, sizeb;
  INT blockstart;

  if (!searchA(&string, &sizea, tmp, sizeof(tmp))) return;
  quit = -1;
  blockstart = base + cursor - sizea + 1;
  do {
    blockstart -= BLOCK_SEARCH_SIZE - sizea + 1;
    sizeb = BLOCK_SEARCH_SIZE;
    if (blockstart < 0) { sizeb -= -blockstart; blockstart = 0; }

    if (sizeb < sizea) quit = -3;
    else {
      if (LSEEK_(fd, blockstart) == -1) { quit = -3; break; }
      if (sizeb != read(fd, tmp, sizeb)) quit = -3;
      else if (getch() != ERR) quit = -2;
      else if ((p = mymemrmem(tmp, sizeb, string, sizea))) quit = p - tmp;
    }

    sprintf(tmpstr,"searching... 0x%08llX", (long long) blockstart);
    nodelay(stdscr, TRUE);
    displayTwoLineMessage(tmpstr, "(press any key to cancel)");

  } while (quit == -1);

  searchB(quit + (quit >= 0 ? blockstart : 0), string);
}


