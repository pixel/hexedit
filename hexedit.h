#ifndef HEXEDIT_H
#define HEXEDIT_H

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <curses.h>
#include <ctype.h>
#include <signal.h>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <errno.h>
#if HAVE_LIBGEN_H
#include <libgen.h>
#endif
#if HAVE_SYS_MOUNT_H
#include <sys/mount.h> /* for BLKGETSIZE */
#endif


#define INT off_t

/*******************************************************************************/
/* Macros */
/*******************************************************************************/
#define BIGGEST_COPYING (1 * 1024 * 1024)
#define BLOCK_SEARCH_SIZE (4 * 1024)
#define SECTOR_SIZE ((INT) 512)
#ifndef CTRL
  #define CTRL(c) ((c) & 0x1F)
#endif
#define ALT(c) ((c) | 0xa0)
#define DIE(M) { fprintf(stderr, M, progName); exit(1); }
#define FREE(p) if (p) free(p)
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define NORMAL A_NORMAL
#define MARKED A_REVERSE
#define MODIFIED A_BOLD
#define ATTRPRINTW(attr, a) do { if (oldattr != (attr)) attrset(attr), oldattr = (attr); printw a; } while (0)
#define MAXATTRPRINTW(attr, a) do { if (oldattr & (~(attr))) attrset(attr & oldattr), oldattr &= (attr); printw a; } while (0)
#define PRINTW(a) ATTRPRINTW(NORMAL, a)
#ifndef getnstr
  #define getnstr(str, n) wgetnstr(stdscr, str, n)
#endif


/*******************************************************************************/
/* Configuration parameters */
/*******************************************************************************/
typedef enum { bySector, maximized, LAST } modeType;
typedef struct {
  int blocSize, lineLength, page;
} modeParams;

extern modeParams modes[LAST];
extern modeType mode;
extern int colored;
extern char *usage;

#define pressAnyKey "(press any key)"


/*******************************************************************************/
/* Pages handling */
/*******************************************************************************/
typedef struct _typePage {
  struct _typePage *next;

  INT base;
  int size;
  unsigned char *vals;
} typePage;


/*******************************************************************************/
/* Global variables */
/*******************************************************************************/

extern INT lastEditedLoc, biggestLoc, fileSize;
extern INT mark_min, mark_max, mark_set;
extern INT base, oldbase;
extern int normalSpaces, cursor, cursorOffset, hexOrAscii;
extern int cursor, blocSize, lineLength, colsUsed, page;
extern int isReadOnly, fd, nbBytes, oldcursor, oldattr, oldcursorOffset;
extern int sizeCopyBuffer, *bufferAttr;
extern char *progName, *fileName, *baseName;
extern unsigned char *buffer, *copyBuffer;
extern typePage *edited;

extern char *lastFindFile, *lastYankToAFile, *lastAskHexString, *lastAskAsciiString, *lastFillWithStringHexa, *lastFillWithStringAscii;


/*******************************************************************************/
/* Miscellaneous functions declaration */
/*******************************************************************************/
INT getfilesize(void);
int key_to_function(int key);
void init(void);
void quit(void);
int tryloc(INT loc);
void openFile(void);
void readFile(void);
int findFile(void);
int computeLineSize(void);
int computeCursorXCurrentPos(void);
int computeCursorXPos(int cursor, int hexOrAscii);
void updateMarked(void);
int ask_about_save(void);
int ask_about_save_and_redisplay(void);
void ask_about_save_and_quit(void);
int setTo(int c);
void setToChar(int i, unsigned char c);

/*******************************************************************************/
/* Pages handling functions declaration */
/*******************************************************************************/
void discardEdited(void);
void addToEdited(INT base, int size, unsigned char *vals);
void removeFromEdited(INT base, int size);
typePage *newPage(INT base, int size);
void freePage(typePage *page);


/*******************************************************************************/
/* Cursor manipulation function declarations */
/*******************************************************************************/
int move_cursor(INT delta);
int set_cursor(INT loc);
int move_base(INT delta);
int set_base(INT loc);

/*******************************************************************************/
/* Curses functions declaration */
/*******************************************************************************/
void initCurses(void);
void exitCurses(void);
void display(void);
void displayLine(int offset, int max);
void clr_line(int line);
void displayCentered(char *msg, int line);
void displayOneLineMessage(char *msg);
void displayTwoLineMessage(char *msg1, char *msg2);
void displayMessageAndWaitForKey(char *msg);
int displayMessageAndGetString(char *msg, char **last, char *p, int p_size);
void ungetstr(char *s);
int get_number(INT *i);


/*******************************************************************************/
/* Search functions declaration */
/*******************************************************************************/
void search_forward(void);
void search_backward(void);


/*******************************************************************************/
/* Mark functions declaration */
/*******************************************************************************/
void markRegion(INT a, INT b);
void unmarkRegion(INT a, INT b);
void markSelectedRegion(void);
void unmarkAll(void);
void markIt(int i);
void unmarkIt(int i);
void copy_region(void);
void yank(void);
void yank_to_a_file(void);
void fill_with_string(void);


/*******************************************************************************/
/* Small common functions declaration */
/*******************************************************************************/
int streq(const char *s1, const char *s2);
int strbeginswith(const char *a, const char *prefix);
int myceil(int a, int b);
INT myfloor(INT a, INT b);
int setLowBits(int p, int val);
int setHighBits(int p, int val);
char *strconcat3(char *a, char *b, char *c);
int hexCharToInt(int c);
int not(int b);
char *mymemmem(char *a, int sizea, char *b, int sizeb);
char *mymemrmem(char *a, int sizea, char *b, int sizeb);
int is_file(char *name);
int hexStringToBinString(char *p, int *l);

/*******************************************************************************/
/* Functions provided for OSs that don't have them */
/*******************************************************************************/
void LSEEK(int fd, INT where);
int LSEEK_(int fd, INT where);

#ifndef HAVE_DECL_MEMRCHR
char *memrchr(void *s, char c, int n);
#endif

#ifndef HAVE_DECL_MEMMEM
void *memmem(void *a, size_t sizea, void *b, size_t sizeb);
#endif

#ifndef HAVE_DECL_MEMRMEM
void *memrmem(void *a, size_t sizea, void *b, size_t sizeb);
#endif

#ifndef HAVE_DECL_BASENAME
char *basename(const char *file);
#endif

#ifndef HAVE_STRERROR
char *strerror(int errnum);
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *str)
#endif

#ifndef HAVE_MEMCMP
#define memcmp(s1, s2, n) bcmp(s2, s1, n)
#endif

#endif  /* HEXEDIT_H */
