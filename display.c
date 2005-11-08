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

int move_cursor(INT delta)
{
  return set_cursor(base + cursor + delta);
}

int set_cursor(INT loc)
{
  if (loc < 0 && base % lineLength)
    loc = 0;

  if (!tryloc(loc))
    return FALSE;

  if (loc < base) {
    if (loc - base % lineLength < 0)
      set_base(0);
    else if (!move_base(myfloor(loc - base % lineLength, lineLength) + base % lineLength - base))
      return FALSE;
    cursor = loc - base;
  } else if (loc >= base + page) {
    if (!move_base(myfloor(loc - base % lineLength, lineLength) + base % lineLength - page + lineLength - base))
      return FALSE;
    cursor = loc - base;
  } else if (loc > base + nbBytes) {
    return FALSE;
  } else
    cursor = loc - base;

  if (mark_set)
    updateMarked();

  return TRUE;
}

int move_base(INT delta)
{
  if (mode == bySector) {
    if (delta > 0 && delta < page)
      delta = page;
    else if (delta < 0 && delta > -page)
      delta = -page;
  }
  return set_base(base + delta);
}

int set_base(INT loc)
{
  if (loc < 0) loc = 0;

  if (!tryloc(loc)) return FALSE;
  base = loc;
  readFile();

  if (mode != bySector && nbBytes < page - lineLength && base != 0) {
    base -= myfloor(page - nbBytes - lineLength, lineLength);
    if (base < 0) base = 0;
    readFile();
  }

  if (cursor > nbBytes) cursor = nbBytes;
  return TRUE;
}


int computeLineSize(void) { return computeCursorXPos(lineLength - 1, 0) + 1; }
int computeCursorXCurrentPos(void) { return computeCursorXPos(cursor, hexOrAscii); }
int computeCursorXPos(int cursor, int hexOrAscii)
{
  int r = 11;
  int x = cursor % lineLength;
  int h = (hexOrAscii ? x : lineLength - 1);

  r += normalSpaces * (h % blocSize) + (h / blocSize) * (normalSpaces * blocSize + 1) + (hexOrAscii && cursorOffset);

  if (!hexOrAscii) r += x + normalSpaces + 1;

  return r;
}



/*******************************************************************************/
/* Curses functions */
/*******************************************************************************/
void initCurses(void)
{
  initscr();

#ifdef HAVE_COLORS
  if (colored) {
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);   /* null zeros */
    init_pair(2, COLOR_GREEN, -1); /* control chars */
    init_pair(3, COLOR_BLUE, -1);  /* extended chars */
  }
#endif

  refresh();
  raw();
  noecho();
  keypad(stdscr, TRUE);

  if (mode == bySector) {
    lineLength = modes[bySector].lineLength;
    page = modes[bySector].page;
    page = myfloor((LINES - 1) * lineLength, page);
    blocSize = modes[bySector].blocSize;
    if (computeLineSize() > COLS) DIE("%s: term is too small for sectored view (width)\n");
    if (page == 0) DIE("%s: term is too small for sectored view (height)\n");
  } else { /* mode == maximized */
    if (LINES <= 4) DIE("%s: term is too small (height)\n");

    blocSize = modes[maximized].blocSize;
    for (lineLength = blocSize; computeLineSize() <= COLS; lineLength += blocSize);
    lineLength -= blocSize;
    if (lineLength == 0) DIE("%s: term is too small (width)\n");

    page = lineLength * (LINES - 1);
  }
  colsUsed = computeLineSize();
  buffer = malloc(page);
  bufferAttr = malloc(page * sizeof(*bufferAttr));
}

void exitCurses(void)
{
  close(fd);
  clear();
  refresh();
  endwin();
}

void display(void)
{
  int i;

  for (i = 0; i < nbBytes; i += lineLength) {
    move(i / lineLength, 0);
    displayLine(i, nbBytes);
  }
  for (; i < page; i += lineLength) {
    int j;
    move(i / lineLength, 0);
    for (j = 0; j < colsUsed; j++) printw(" "); /* cleanup the line */
    move(i / lineLength, 0);
    PRINTW(("%08lX", (int) (base + i)));
  }

  attrset(NORMAL);
  move(LINES - 1, 0);
  for (i = 0; i < colsUsed; i++) printw("-");
  move(LINES - 1, 0);
  if (isReadOnly) i = '%';
  else if (edited) i = '*';
  else i = '-';
  printw("-%c%c  %s       --0x%llX", i, i, baseName, base + cursor);
  if (MAX(fileSize, lastEditedLoc)) printw("/0x%llX", getfilesize());
  if (mode == bySector) printw("--sector %d", (base + cursor) / SECTOR_SIZE);

  move(cursor / lineLength, computeCursorXCurrentPos());
}

void displayLine(int offset, int max)
{
  int i;

  PRINTW(("%08lX   ", (int) (base + offset)));
  for (i = offset; i < offset + lineLength; i++) {
    if (i > offset) MAXATTRPRINTW(bufferAttr[i] & MARKED, (((i - offset) % blocSize) ? " " : "  "));
    if (i < max) {
      ATTRPRINTW(
#ifdef HAVE_COLORS
		 (!colored ? 0 :
		  buffer[i] == 0 ? COLOR_PAIR(1) :
		  buffer[i] < ' ' ? COLOR_PAIR(2) : 
		  buffer[i] >= 127 ? COLOR_PAIR(3) : 0) |
#endif
		 bufferAttr[i], ("%02X", buffer[i]));
    }
    else PRINTW(("  "));
  }
  PRINTW(("  "));
  for (i = offset; i < offset + lineLength; i++) {
    if (i >= max) PRINTW((" "));
    else if (buffer[i] >= ' ' && buffer[i] < 127) ATTRPRINTW(bufferAttr[i], ("%c", buffer[i]));
    else ATTRPRINTW(bufferAttr[i], ("."));
  }
}

void clr_line(int line) { move(line, 0); clrtoeol(); }

void displayCentered(char *msg, int line)
{
  clr_line(line);
  move(line, (COLS - strlen(msg)) / 2);
  PRINTW(("%s", msg));
}

void displayOneLineMessage(char *msg)
{
  int center = page / lineLength / 2;
  clr_line(center - 1);
  clr_line(center + 1);
  displayCentered(msg, center);
}

void displayTwoLineMessage(char *msg1, char *msg2)
{
  int center = page / lineLength / 2;
  clr_line(center - 2);
  clr_line(center + 1);
  displayCentered(msg1, center - 1);
  displayCentered(msg2, center);
}

void displayMessageAndWaitForKey(char *msg)
{
  displayTwoLineMessage(msg, pressAnyKey);
  getch();
}

int displayMessageAndGetString(char *msg, char **last, char *p, int p_size)
{
  int ret = TRUE;

  displayOneLineMessage(msg);
  ungetstr(*last);
  echo();
  getnstr(p, p_size - 1);
  noecho();
  if (*p == '\0') {
    if (*last) strcpy(p, *last); else ret = FALSE;
  } else {
    FREE(*last);
    *last = strdup(p);
  }
  return ret;
}

void ungetstr(char *s)
{
  char *p;
  if (s)
    for (p = s + strlen(s) - 1; p >= s; p--) ungetch(*p);
}

int get_number(INT *i)
{
  int err;
  char tmp[BLOCK_SEARCH_SIZE];
  echo();
  getnstr(tmp, BLOCK_SEARCH_SIZE - 1);
  noecho();
  if (strbeginswith(tmp, "0x"))
    err = sscanf(tmp + strlen("0x"), "%llx", i);
  else
    err = sscanf(tmp, "%lld", i);
  return err == 1;
}
