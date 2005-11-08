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


int LSEEK_(int fd, INT where)
{
  INT result;

  result = lseek(fd, where, SEEK_SET);
  return (result == where) ? 1 : -1;
}

void LSEEK(int fd, INT where)
{
  INT result;

  result = lseek(fd, where, SEEK_SET);
  if (result != where) {
    exitCurses();
    fprintf(stderr, "the long seek failed (%lld instead of %lld), leaving :(\n", (long long) result, (long long) where);
    exit(1);
  }
}

/*******************************************************************************/
/* Functions provided for OSs that don't have them */
/*******************************************************************************/
#ifndef HAVE_BASENAME
char *basename(char *file) {
  char *p = strrchr(file, '/');
  return p ? p + 1 : file;
}
#endif

#ifndef HAVE_STRERROR
char *strerror(int errnum) {
  extern char *sys_errlist[];
  extern int sys_nerr;

  if (errnum > 0 && errnum <= sys_nerr)
    return sys_errlist[errnum];
  return _("Unknown system error");
}
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *str)
{
  size_t len = strlen(str) + 1;
  void *new = malloc(len);
  if (new == NULL) return NULL;

  return (char *) bcopy(str, new, len);
}
#endif

/*******************************************************************************/
/* Small common functions */
/*******************************************************************************/
int streq(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }
INT myfloor(INT a, INT b) { return a - a % b; }
int setLowBits(int p, int val) { return (p & 0xF0) + val; }
int setHighBits(int p, int val) { return (p & 0x0F) + val * 0x10; }

int strbeginswith(const char *a, const char *prefix)
{
  return strncmp(a, prefix, strlen(prefix)) == 0;
}

char *strconcat3(char *a, char *b, char *c)
{
  size_t la = a ? strlen(a) : 0;
  size_t lb = b ? strlen(b) : 0;
  size_t lc = c ? strlen(c) : 0;
  char *p = malloc(la + lb + lc + 1);
  if (a) memcpy(p, a, la);
  if (b) memcpy(p + la, b, lb);
  if (c) memcpy(p + la + lb, c, lc);
  p[la + lb + lc] = '\0';
  return p;
}

int hexCharToInt(int c)
{
  if (isdigit(c)) return c - '0';
  return tolower(c) - 'a' + 10;
}

int not(int b) { return b ? FALSE: TRUE; }

#ifndef HAVE_MEMRCHR
void *memrchr(const void *s, int c, size_t n)
{
  int i;
  const char *cs = s;
  for (i = n - 1; i >= 0; i--) if (cs[i] == c) return (void *) &cs[i];
  return NULL;
}
#endif

char *mymemmem(char *a, int sizea, char *b, int sizeb)
{
#ifdef HAVE_MEMMEM
  return memmem(a, sizea, b, sizeb);
#else
  char *p;
  int i = sizea - sizeb + 1;
  
  for (; (p = memchr(a, b[0], i)); i -= p - a + 1, a = p + 1)
  {
    if ((memcmp(p + 1, b + 1, sizeb - 1)) == 0) {
	 return p;
    }
  }
  return NULL;
#endif
}

char *mymemrmem(char *a, int sizea, char *b, int sizeb)
{
#ifdef HAVE_MEMRMEM
  return memrmem(a, sizea, b, sizeb);
#else
  char *p;
  int i = sizea - sizeb + 1;
  
  a += sizea - 1;
  for (; (p = memrchr(a - i + 1, b[sizeb - 1], i)); i -= a - p + 1, a = p - 1)
  {
    if ((memcmp(p - sizeb + 1, b, sizeb - 1)) == 0) return p;
  }
  return NULL;
#endif
}


int hexStringToBinString(char *p, int *l)
{
  int i;

  for (i = 0; i < *l; i++) {
    if (!isxdigit(p[i])) {
      displayMessageAndWaitForKey("Invalid hexa string");
      return FALSE;
    }
    p[i / 2] = ((i % 2) ? setLowBits : setHighBits)(p[i / 2], hexCharToInt(p[i]));
  }

  if ((*l % 2)) {
    displayMessageAndWaitForKey("Must be an even number of chars");
    return FALSE;
  }
  *l /= 2;
  return TRUE;
}

