/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007-2008 Benjamin Zores <ben@geexbox.org>
 *
 * This file is part of libdlna.
 *
 * libdlna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libdlna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libdlna; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "buffer.h"
#include "minmax.h"

#define BUFFER_DEFAULT_CAPACITY 32768

buffer_t *
buffer_new (void)
{
  buffer_t *buffer = NULL;

  buffer = malloc (sizeof (buffer_t));
  buffer->buf = NULL;
  buffer->len = 0;
  buffer->capacity = 0;

  return buffer;
}

void
buffer_append (buffer_t *buffer, const char *str)
{
  size_t len;

  if (!buffer || !str)
    return;

  if (!buffer->buf)
  {
    buffer->capacity = BUFFER_DEFAULT_CAPACITY;
    buffer->buf = malloc (buffer->capacity);
    memset (buffer->buf, '\0', buffer->capacity);
  }

  len = buffer->len + strlen (str);
  if (len >= buffer->capacity)
  {
    buffer->capacity = MAX (len + 1, 2 * buffer->capacity);
    buffer->buf = realloc (buffer->buf, buffer->capacity);
  }

  strcat (buffer->buf, str);
  buffer->len += strlen (str);
}

void
buffer_appendf (buffer_t *buffer, const char *format, ...)
{
  char str[BUFFER_DEFAULT_CAPACITY];
  int size;
  va_list va;

  if (!buffer || !format)
    return;

  va_start (va, format);
  size = vsnprintf (str, BUFFER_DEFAULT_CAPACITY, format, va);
  if (size >= BUFFER_DEFAULT_CAPACITY)
  {
    char *dynstr = malloc (size + 1);
    vsnprintf (dynstr, size + 1, format, va);
    buffer_append (buffer, dynstr);
    free (dynstr);
  }
  else
    buffer_append (buffer, str);
  va_end (va);
}

void
buffer_free (buffer_t *buffer)
{
  if (!buffer)
    return;

  if (buffer->buf)
    free (buffer->buf);
  free (buffer);
}
