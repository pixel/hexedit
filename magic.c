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
#if HAVE_MAGIC
#include <magic.h>

static magic_t m;

/*******************************************************************************/
/* interactive functions */
/*******************************************************************************/
void initMagic(void)
{
  m = magic_open(0);
  magic_load(m, NULL);
}

void evalMagic(void)
{
  const char * magic = magic_buffer(m, buffer + cursor, page - cursor);

  if( !magic )
    magic = magic_error(m);

  displayMessageAndWaitForKey(magic);
}

void freeMagic(void)
{
  magic_close(m);
}

#endif /*HAVE_MAGIC*/
