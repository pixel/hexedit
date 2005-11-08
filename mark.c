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
/* Mark functions */
/*******************************************************************************/
void markRegion(INT a, INT b) { int i; for (i = MAX(a - base, 0); i <= MIN(b - base, nbBytes - 1); i++) markIt(i); }
void unmarkRegion(INT a, INT b) { int i; for (i = MAX(a - base, 0); i <= MIN(b - base, nbBytes - 1); i++) unmarkIt(i); }
void markSelectedRegion(void) { markRegion(mark_min, mark_max); }
void unmarkAll(void) { unmarkRegion(base, base + nbBytes - 1); }
void markIt(int i) { bufferAttr[i] |= MARKED; }
void unmarkIt(int i) { bufferAttr[i] &= ~MARKED; }

void copy_region(void) 
{
  typePage *p;

  if (!mark_set) { displayMessageAndWaitForKey("Nothing to copy"); return; }
  sizeCopyBuffer = mark_max - mark_min + 1;
  if (sizeCopyBuffer == 0) return;
  if (sizeCopyBuffer > BIGGEST_COPYING) {
    displayTwoLineMessage("Hey, don't you think that's too big?!", "Really copy (Yes/No)");
    if (tolower(getch()) != 'y') return;
  }
  FREE(copyBuffer);
  if ((copyBuffer = malloc(sizeCopyBuffer)) == NULL) {
    displayMessageAndWaitForKey("Can't allocate that much memory");
    return;
  }
  if (LSEEK_(fd, mark_min) == -1 || read(fd, copyBuffer, sizeCopyBuffer) == -1) {
    displayMessageAndWaitForKey(strerror(errno));
    return;
  }

  for (p = edited; p; p = p->next) {
    if (mark_min < p->base + p->size && p->base <= mark_max) {
      INT min = MIN(p->base, mark_min);
      memcpy(copyBuffer + p->base - min, 
	     p->vals + mark_min - min,
	     MIN(p->base + p->size, mark_max) - MAX(p->base, mark_min) + 1);
    }
  }
  unmarkAll();
  mark_set = FALSE;
}

void yank(void) 
{
  if (copyBuffer == NULL) { displayMessageAndWaitForKey("Nothing to paste"); return; }
  if (isReadOnly) { displayMessageAndWaitForKey("File is read-only!"); return; }
  addToEdited(base + cursor, sizeCopyBuffer, copyBuffer);
  readFile();
}

void yank_to_a_file(void) 
{
  char tmp[BLOCK_SEARCH_SIZE];
  int f;

  if (copyBuffer == NULL) { displayMessageAndWaitForKey("Nothing to paste"); return; }

  if (!displayMessageAndGetString("File name: ", &lastYankToAFile, tmp, sizeof(tmp))) return;
  
  if ((f = open(tmp, O_RDONLY)) != -1) {
    close(f);
    displayTwoLineMessage("File exists", "Overwrite it (Yes/No)");
    if (tolower(getch()) != 'y') return;
  }
  if ((f = creat(tmp, 0666)) == -1 || write(f, copyBuffer, sizeCopyBuffer) == -1) {
    displayMessageAndWaitForKey(strerror(errno));
    return;
  }

  close(f);
}

void fill_with_string(void)
{
  char *msg = hexOrAscii ? "Hexa string to fill with: " : "Ascii string to fill with: ";
  char **last = hexOrAscii ? &lastFillWithStringHexa : &lastFillWithStringAscii;
  char tmp2[BLOCK_SEARCH_SIZE];
  unsigned char *tmp1;
  int i, l1, l2;

  if (!mark_set) return;
  if (isReadOnly) { displayMessageAndWaitForKey("File is read-only!"); return; }
  if (sizeCopyBuffer > BIGGEST_COPYING) {
    displayTwoLineMessage("Hey, don't you think that's too big?!", "Really fill (Yes/No)");
    if (tolower(getch()) != 'y') return;
  }  
  if (!displayMessageAndGetString(msg, last, tmp2, sizeof(tmp2))) return;
  l1 = mark_max - mark_min + 1;
  l2 = strlen(tmp2);
  if (hexOrAscii) {
    if (strlen(tmp2) == 1) {
      if (!isxdigit(*tmp2)) { displayMessageAndWaitForKey("Invalid hexa string"); return; }
      *tmp2 = hexCharToInt(*tmp2);
    } else if (!hexStringToBinString(tmp2, &l2)) return;
  }
  tmp1 = malloc(l1);
  for (i = 0; i < l1 - l2 + 1; i += l2) memcpy(tmp1 + i, tmp2, l2);
  memcpy(tmp1 + i, tmp2, l1 - i);
  addToEdited(mark_min, l1, tmp1);
  readFile();
  free(tmp1);
}


void updateMarked(void)
{
  if (base + cursor > oldbase + oldcursor) {

    if (mark_min == mark_max) {
      mark_max = base + cursor;
    } else if (oldbase + oldcursor == mark_min) {
      if (base + cursor <= mark_max) {
	mark_min = base + cursor;
	unmarkRegion(oldbase + oldcursor, mark_min - 1);
      } else {
	unmarkRegion(oldbase + oldcursor, mark_max);
	mark_min = mark_max;
	mark_max = base + cursor;
      }
    } else {
      mark_max = base + cursor;
    }

  } else if (base + cursor < oldbase + oldcursor){
    if (mark_min == mark_max) {
      mark_min = base + cursor;
    } else if (oldbase + oldcursor == mark_max) {
      if (base + cursor >= mark_min) {
	mark_max = base + cursor;
	unmarkRegion(mark_max + 1, oldbase + oldcursor);
      } else {
	unmarkRegion(mark_min, oldbase + oldcursor);
	markRegion(base + cursor, mark_min - 1);
	mark_max = mark_min;
	mark_min = base + cursor;
      }
    } else {
      mark_min = base + cursor;
    }
  }
  if (mark_max >= getfilesize()) mark_max = getfilesize() - 1;
  markSelectedRegion();
}
