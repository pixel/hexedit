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


// Compute number of hex-digits needed to display an address.
// We only increase this by full bytes (2 digits).
// Because the input is uint64_t, the maximum result is 16 (64bit = 16*4bit).
int compute_nDigits(uint64_t maxAddr)
{
  int digits = 0;
  while (maxAddr) {
    digits+=2;
    maxAddr >>= 8;
  }

  if (digits==0) {
    return 2;
  }

  return digits;
}


void openFile(void)
{
  struct stat st;

  if (!is_file(fileName)) {
    fprintf(stderr, "%s: %s: Not a file.\n", progName, fileName);
    exit(1);
  }

  /* edited should be cleaned here (assert(edited == NULL)) */
  if (isReadOnly || (fd = open(fileName, O_RDWR)) == -1) {
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

  nAddrDigits = compute_nDigits(biggestLoc);

  // use at least 8 digits (4 byte addresses)
  if (nAddrDigits < 8) {
    nAddrDigits = 8;
  }
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
  if (!is_file(tmp)) {
    FREE(p);
    return FALSE;
  }
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

void openTagFile(void)
{
  // tagFile 0 - doesn't exist, 1 - problem, 2 - read only, 3 - read write

  char tagFileName[BLOCK_SEARCH_SIZE];
  snprintf(tagFileName, BLOCK_SEARCH_SIZE, "%s.tags", fileName);

  if (!is_file(tagFileName))
    tagFile = 0;
  else {
    tagfd = fopen(tagFileName, "rw");
    if (tagfd != NULL)
      tagFile = 3;
    else
    {
      tagfd = fopen(tagFileName, "r");
      tagFile = tagfd != NULL ? 2 : 1;
    }
  }

  if (tagfd != NULL && tagFile > 1)
    readTagFile();
}

void readTagFile(void)
{
  char raw[BLOCK_SEARCH_SIZE], *line, *token;

  while (fgets(raw, BLOCK_SEARCH_SIZE, tagfd) != NULL)
  {
    line = (char*)&raw;
    int loc = 0;

    token = strsep(&line, " ");
    if (strncmp(token, "0x0", BLOCK_SEARCH_SIZE) == 0)
      loc = 0;
    else
    {
      loc = (int)strtol(token, NULL, 16);
      if ( loc == 0) //Couldn't covert the position, skip the line
        continue;
    }

    token = strsep(&line, " "); 
    if ( token == NULL ) //Couldn't find a token skip rest of line
      continue;

    // Note n:
    if (strncmp(token,"n:",BLOCK_SEARCH_SIZE) == 0)
    {
      char *note = strsep(&line, "\n");
      if (loc > notes_size)
      {
        notes_size = loc+NOTE_SIZE;
        notes = (noteStruct*) realloc(notes,notes_size*sizeof(noteStruct));
      }
      notes[loc].note = (char*) malloc(NOTE_SIZE);
      memset(notes[loc].note,'\0',NOTE_SIZE);
      snprintf(notes[loc].note,NOTE_SIZE,"%s",note);
    }

    // Colour c: 1-CYAN, 2-MAGENTA, 3-YELLOW
    if (strncmp(token,"c:",BLOCK_SEARCH_SIZE) == 0)
    {
      char *restofline;
      int end = 0, color = 0;
      int col = (int)strtol(line, &restofline, 10);
      if (col < 1 || col > 3) //conversion error or out of range
        continue;
      if (restofline)
        end = (int)strtol(restofline, NULL, 10);

      switch (col) {
        case 1: color = (int) COLOR_PAIR(5); break;
        case 2: color = (int) COLOR_PAIR(6); break;
        case 3: color = (int) COLOR_PAIR(7); break;
      }
      if (end != 0)
        for (int i = loc; i < loc+end; i++)
        {
          int m = bufferAttr[i] & MARKED;
          int b = bufferAttr[i] & A_BOLD;
          bufferAttr[i] = color;
          bufferAttr[i] |= TAGGED;
          bufferAttr[i] |= m;
          bufferAttr[i] |= b;
        }
      else
      {
        int m = bufferAttr[loc] & MARKED;
        int b = bufferAttr[loc] & A_BOLD;
        bufferAttr[loc] = color;
        bufferAttr[loc] |= TAGGED;
        bufferAttr[loc] |= m;
        bufferAttr[loc] |= b;
      }
    } 
  }
}

void writeTagFile(void)
{
  if (tagFile == 1 || tagFile == 2) return; // tag file not writeable

  if (tagFile == 3 || tagFile == 0) // writeable and open
  {
    displayOneLineMessage("Write tags to file? (y/N)");
    if (tolower(getch()) != 'y') return;

    char tagFileName[BLOCK_SEARCH_SIZE];
    snprintf(tagFileName, BLOCK_SEARCH_SIZE, "%s.tags", fileName);
    tagfd = freopen(tagFileName, "w", tagfd);

    int start=0, end=0, col=0, lastcol=0;
    for (int i = 0; i < fileSize; i++)
    {
      if (notes[i].note) fprintf(tagfd, "0x%x n: %s\n", i, notes[i].note);
      if (bufferAttr[i] & TAGGED)
      {
        col = (bufferAttr[i] & COLOR_PAIR(7)) == COLOR_PAIR(7) ? 3 : 
              (bufferAttr[i] & COLOR_PAIR(6)) == COLOR_PAIR(6) ? 2 : 
              (bufferAttr[i] & COLOR_PAIR(5)) == COLOR_PAIR(5) ? 1 : 0;

        if ( start != 0 && i == end+1 && col == lastcol )
          end = i;
        if ( start != 0 && i == end+1 && col != lastcol )
        {
          fprintf(tagfd, "0x%x c: %d %d\n", start, lastcol, 1+end-start);
          start = i;
          end = i;
        }
        if ( start == 0 )
        {
          start = i;
          end = i;
        }
        lastcol = col;
      }
      else
      {
        if ( start != 0 && i == end+1 )
        {
          fprintf(tagfd, "0x%x c: %d %d\n", start, lastcol, 1+end-start);
          start = 0;
          end = 0;
        }
      }
    }

    fclose(tagfd);
  }
}
