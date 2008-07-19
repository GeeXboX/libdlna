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

#ifndef BUFFER_H
#define BUFFER_H

struct buffer_t {
  char *buf;
  size_t len;
  size_t capacity;
};

struct buffer_t *buffer_new (void) __attribute__ ((malloc));
void buffer_free (struct buffer_t *buffer);

void buffer_append (struct buffer_t *buffer, const char *str);
void buffer_appendf (struct buffer_t *buffer, const char *format, ...)
    __attribute__ ((format (printf , 2, 3)));

#endif /* BUFFER_H */
