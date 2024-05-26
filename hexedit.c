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
int fd, nbBytes, oldcursor, oldattr, oldcursorOffset;
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
int isReadOnly = FALSE;
int disableOpts = FALSE;

const char * const usage = "usage: %s [-s | --sector] [-m | --maximize] [-l<n> | --linelength <n>] [-r | --readonly]"
#ifdef HAVE_COLORS 
     " [-c | --color]"
#endif 
     " [-h | --help] filename\n";

typedef void (*ParseArgCallbackFn)(const char *nextArg);

static void pacallback_sector      (const char *unused);
static void pacallback_readOnly    (const char *unused);
static void pacallback_maximize    (const char *unused);
#ifdef HAVE_COLORS
static void pacallback_color       (const char *unused);
#endif
static void pacallback_lineLength  (const char *nextArg);
static void pacallback_disableOpts (const char *unused);
static void pacallback_usage       (const char *unused);

static inline int parseArg_matchOpt (const char *arg, const char *shortName, const char *longName);
static inline int parseArg          (const char **argv);

/* name:    pacallback_*
 * nextArg: argument for option, if applicable
 * desc:    these are callbacks per ParseArgCallbackFn, used by parseArg
 */
static void pacallback_sector(const char *unused)
{
  (void) unused;
  mode = bySector;
}

static void pacallback_readOnly(const char *unused)
{
  (void) unused;
  isReadOnly = TRUE;
}

static void pacallback_maximize(const char *unused)
{
  (void) unused;
  mode = maximized;
  lineLength = 0;
}

#ifdef HAVE_COLORS
static void pacallback_color(const char *unused)
{
  (void) unused;
  colored = TRUE;
}
#endif

static void pacallback_lineLength(const char *nextArg)
{
  lineLength = atoi(nextArg);

  if (lineLength < 0 || lineLength > 4096)
    DIE("%s: illegal line length\n")
}

static void pacallback_disableOpts(const char *unused)
{
  (void) unused;
  disableOpts = TRUE;
}

static void pacallback_usage(const char *unused)
{
  (void) unused;
  DIE(usage);
}

/* name:      parseArg_matchOpt
 * arg:       current argument
 * shortName: short name to check against
 * longName:  long name to check against
 * return:    number indicating status
 *            0: no match
 *            1: short match
 *            2: long match
 */
static inline int parseArg_matchOpt(const char *arg, const char *shortName, const char *longName)
{
  if (shortName != NULL && strbeginswith(arg, shortName))
    return 1;

  if (longName != NULL && strbeginswith(arg, longName))
    return 2;

  return 0;
}

/* name:   parseArg
 * argvp:  pointer to current argv offset
 * return: number of arguments to advance
 * desc:   parses arguments according to embedded argument list.
 *         Execution is handled by callback functions.
 * notes:
 *
 * 1. This can be canonized as a type in a future update, but is left
 *    anonymous for now
 *
 * 2. Adds support for the --longopt=value convention
 *
 * 3. if a filename was already selected, free the existing one and
 *    replace it. Note that file scope variables are guaranteed to be
 *    zero initialized per the standard.
 *
 * 4. Fixes issue #70 -- if no options match the argument, it is assumed
 *    to be the filename. The effect is that the most recent "naked"
 *    argument is used as the filename.
 */
static inline int parseArg(const char **argvp)
{
  static const struct { // 1
    const char *       shortName;
    const char *       longName;
    int                hasArg;
    ParseArgCallbackFn callback;
  } opts[] = {
    {"-s", "--sector",     FALSE, pacallback_sector},
    {"-r", "--readonly",   FALSE, pacallback_readOnly},
    {"-m", "--maximize",   FALSE, pacallback_maximize},
#ifdef HAVE_COLORS
    {"-c", "--color",      FALSE, pacallback_color},
#endif
    {"-l", "--linelength", TRUE,  pacallback_lineLength},
    {"-h", "--help",       FALSE, pacallback_usage},
    {"--", NULL,           FALSE, pacallback_disableOpts},
    {"-",  NULL,           FALSE, pacallback_usage},
  };
  static const size_t opts_len = sizeof opts / sizeof *opts;

  const char *curArg = argvp[0];
  const char *nextArg = NULL;
  int advanceBy = 1;

  if (disableOpts)
    goto exit_disableOpts;

  for (size_t i = 0; i < opts_len; ++i)
  {
    int matchOpt = parseArg_matchOpt(curArg, opts[i].shortName, opts[i].longName);
    const char *strchrMatch;

    if (matchOpt == 0)
      continue;

    if (opts[i].hasArg)
    {
      if (matchOpt == 1 && strlen(curArg) > 2)
        nextArg = curArg + 2;

      else if (matchOpt == 2 && (strchrMatch = strchr(curArg, '=')) != NULL) // 2
        nextArg = strchrMatch + 1;

      else
      {
        nextArg = argvp[1];
        advanceBy = 2;
      }
    }

    opts[i].callback(nextArg);
    return advanceBy;
  }

exit_disableOpts:
  if (fileName != NULL) // 3
    free(fileName);

  fileName = strdup(curArg); // 4
  return 1;
}

/*******************************************************************************/
/* main */
/*******************************************************************************/
int main(int argc, char **argv)
{
  progName = basename(argv[0]);
  argv++; argc--;

  for (; argc > 0;)
  {
    int advanceBy = parseArg((const char **)argv);

    argv += advanceBy;
    argc -= advanceBy;
  }

  init();
  initCurses();
  if (fileName == NULL && !findFile()) {
    exitCurses();
    DIE("%s: No such file\n");
  }
  openFile();
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





