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

void openFile(void)
{
  struct stat st;

  if (!is_file(fileName)) {
    fprintf(stderr, "%s: %s: Not a file.\n", progName, fileName);
    exit(1);
  }

  /* edited should be cleaned here (assert(edited == NULL)) */
  if ((fd = open(fileName, O_RDWR)) == -1) {
    isReadOnly = TRUE;
    if ((fd = open(fileName, O_RDONLY)) == -1) {
      if (page) exitCurses();
      if (fileName[0] == '-') DIE(usage);
      fprintf(stderr, "%s: ", progName);
      perror(fileName);
      exit(1);
    }
  } else isReadOnly = FALSE;
  baseName = basename(fileName);
  mark_set = FALSE;
  lastEditedLoc = base = cursor = cursorOffset = 0;

  /* check file size- doesn't work on devices.  I considered implementing a conquer-and-divide algorithm, but it would unpleasant on tape drives and such. */
  if (fstat(fd, &st) != -1 && st.st_size > 0)
    fileSize = st.st_size;
  else {
#ifdef BLKGETSIZE
    unsigned long i;
    if (ioctl(fd, BLKGETSIZE, &i) == 0)
      fileSize = (INT) i * 512;
    else
#endif
      fileSize = 0;
  }
  biggestLoc = fileSize;
}

void readFile(void)
{
  typePage *p;
  INT i;

  memset(buffer, 0, page * sizeof(*buffer));

  LSEEK(fd, base);
  nbBytes = read(fd, buffer, page);
  if (nbBytes < 0)
    nbBytes = 0;
  else if (nbBytes && base + nbBytes > biggestLoc)
    biggestLoc = base + nbBytes;
  memset(bufferAttr, A_NORMAL, page * sizeof(*bufferAttr));
  for (p = edited; p; p = p->next) {
    for (i = MAX(base, p->base); i < MIN(p->base + p->size, base + page); i++) {
      if (buffer[i - base] != p->vals[i - p->base]) {
	buffer[i - base] = p->vals[i - p->base];
	bufferAttr[i - base] |= MODIFIED;
      }
    }
    if (p->base + p->size > base + nbBytes) {   /* Check for modifications past EOF */
      for(; p->base + p->size > base + nbBytes && nbBytes < page; nbBytes++)
	bufferAttr[nbBytes] |= MODIFIED;
    }
  }
  if (mark_set) markSelectedRegion();

}

int findFile(void)
{
  char *p, tmp[BLOCK_SEARCH_SIZE];
  p = lastFindFile ? strdup(lastFindFile) : NULL;
  if (!displayMessageAndGetString("File name: ", &p, tmp, sizeof(tmp))) return FALSE;
  if (!is_file(tmp)) return FALSE;
  FREE(lastFindFile); lastFindFile = fileName;
  fileName = p;
  return TRUE;
}


INT getfilesize(void)
{
  return MAX(lastEditedLoc, biggestLoc);
}


/* tryloc:
 *   returns TRUE if loc is an apropriate location to place the cursor
 *   assumes the file won't shrink.
 *   returns TRUE if the cursor is one past EOF to allow appending
 */

int tryloc(INT loc)
{
  char c;
  if (loc < 0)
    return FALSE;
  if (loc <= lastEditedLoc)
    return TRUE;

  if (loc <= biggestLoc)
    return TRUE;

  if (LSEEK_(fd, loc - 1) != -1 && /* don't have to worry about loc - 1 < 0 */
      read(fd, &c, 1) == 1) {
    biggestLoc = loc;
    return TRUE;
  }
  return FALSE;
}

int is_file(char *name)
{
  struct stat st;
  return stat(name, &st) != -1 && !S_ISDIR(st.st_mode);
}


