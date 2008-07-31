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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <ffmpeg/common.h>

#include "upnp_internals.h"

#define PROTOCOL_TYPE_PRE_SZ  11   /* for the str length of "http-get:*:" */
#define PROTOCOL_TYPE_SUFF_SZ 2    /* for the str length of ":*" */

typedef enum {
  HTTP_ERROR = -1,
  HTTP_OK    =  0,
} http_error_code_t;

typedef struct http_file_handler_s {
  char *fullpath;
  off_t pos;
  enum {
    HTTP_FILE_LOCAL,
    HTTP_FILE_MEMORY
  } type;
  union {
    struct {
      int fd;
      vfs_item_t *item;
    } local;
    struct {
      char *content;
      off_t len;
    } memory;
  } detail;
} http_file_handler_t;

static inline void
set_service_http_info (struct File_Info *info,
                       const size_t length,
                       const char *content_type)
{
  info->file_length   = length;
  info->last_modified = 0;
  info->is_directory  = 0;
  info->is_readable   = 1;
  info->content_type  = ixmlCloneDOMString (content_type);
}

static int
upnp_http_get_info (void *cookie,
                    const char *filename,
                    struct File_Info *info)
{
  dlna_t *dlna;
  uint32_t id;
  vfs_item_t *item;
  char *content_type;
  char *protocol_info;
  struct stat st;
  
  if (!cookie || !filename || !info)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;

  dlna_log (dlna, DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  /* trap application-level HTTP callback */
  if (dlna->http_callback && dlna->http_callback->get_info)
  {
    dlna_http_file_info_t finfo;
    int err;

    err = dlna->http_callback->get_info (filename, &finfo);
    if (!err)
    {
      set_service_http_info (info, finfo.file_length, finfo.content_type);
      return HTTP_OK;
    }
  }
  
  /* ask for Content Directory Service (CDS) */
  if (!strcmp (filename, CDS_LOCATION))
  {
    set_service_http_info (info, CDS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for Connection Manager Service (CMS) */
  if (!strcmp (filename, CMS_LOCATION))
  {
    set_service_http_info (info, CMS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for AVTransport Service (AVTS) */
  if (!strcmp (filename, AVTS_LOCATION))
  {
    set_service_http_info (info, AVTS_DESCRIPTION_LEN, SERVICE_CONTENT_TYPE);
    return HTTP_OK;
  }

  /* ask for anything else ... */
  id = atoi (strrchr (filename, '/') + 1);
  item = vfs_get_item_by_id (dlna, id);
  if (!item)
    return HTTP_ERROR;

  if (item->type != DLNA_RESOURCE)
    return HTTP_ERROR;

  if (!item->u.resource.fullpath)
    return HTTP_ERROR;

  if (stat (item->u.resource.fullpath, &st) < 0)
    return HTTP_ERROR;

  info->is_readable = 1;
  if (access (item->u.resource.fullpath, R_OK) < 0)
  {
    if (errno != EACCES)
      return HTTP_ERROR;
    info->is_readable = 0;
  }

  /* file exist and can be read */
  info->file_length = st.st_size;
  info->last_modified = st.st_mtime;
  info->is_directory = S_ISDIR (st.st_mode);

  protocol_info = 
    dlna_write_protocol_info (DLNA_PROTOCOL_INFO_TYPE_HTTP,
                              DLNA_ORG_PLAY_SPEED_NORMAL,
                              DLNA_ORG_CONVERSION_NONE,
                              DLNA_ORG_OPERATION_RANGE,
                              dlna->flags, item->u.resource.item->profile);

  content_type =
    strndup ((protocol_info + PROTOCOL_TYPE_PRE_SZ),
             strlen (protocol_info + PROTOCOL_TYPE_PRE_SZ)
             - PROTOCOL_TYPE_SUFF_SZ);
  free (protocol_info);

  if (content_type)
  {
    info->content_type = ixmlCloneDOMString (content_type);
    free (content_type);
  }
  else
    info->content_type = ixmlCloneDOMString ("");
  
  return HTTP_OK;
}

static dlnaWebFileHandle
http_get_file_from_memory (const char *fullpath,
                           const char *description,
                           const size_t length)
{
  dlna_http_file_handler_t *dhdl;
  http_file_handler_t *hdl;

  if (!fullpath || !description || length == 0)
    return NULL;
  
  hdl                        = malloc (sizeof (http_file_handler_t));
  hdl->fullpath              = strdup (fullpath);
  hdl->pos                   = 0;
  hdl->type                  = HTTP_FILE_MEMORY;
  hdl->detail.memory.content = strdup (description);
  hdl->detail.memory.len     = length;

  dhdl                       = malloc (sizeof (dlna_http_file_handler_t));
  dhdl->external             = 0;
  dhdl->priv                 = hdl;
  
  return ((dlnaWebFileHandle) dhdl);
}

static dlnaWebFileHandle
http_get_file_local (vfs_item_t *item)
{
  dlna_http_file_handler_t *dhdl;
  http_file_handler_t *hdl;
  int fd;
  
  if (!item)
    return NULL;

  if (!item->u.resource.fullpath)
    return NULL;
  
  fd = open (item->u.resource.fullpath,
             O_RDONLY | O_NONBLOCK | O_SYNC | O_NDELAY);
  if (fd < 0)
    return NULL;
  
  hdl                        = malloc (sizeof (http_file_handler_t));
  hdl->fullpath              = strdup (item->u.resource.fullpath);
  hdl->pos                   = 0;
  hdl->type                  = HTTP_FILE_LOCAL;
  hdl->detail.local.fd       = fd;
  hdl->detail.local.item     = item;

  dhdl                       = malloc (sizeof (dlna_http_file_handler_t));
  dhdl->external             = 0;
  dhdl->priv                 = hdl;

  return ((dlnaWebFileHandle) dhdl);
}

static dlnaWebFileHandle
upnp_http_open (void *cookie,
                const char *filename,
                enum dlnaOpenFileMode mode)
{
  dlna_t *dlna;
  uint32_t id;
  vfs_item_t *item;
  
  if (!cookie || !filename)
    return NULL;

  dlna = (dlna_t *) cookie;

  dlna_log (dlna, DLNA_MSG_INFO,
            "%s, filename : %s\n", __FUNCTION__, filename);

  if (mode != DLNA_READ)
    return NULL;

  /* trap application-level HTTP callback */
  if (dlna->http_callback && dlna->http_callback->open)
  {
    dlna_http_file_handler_t *dhdl;
    dhdl = dlna->http_callback->open (filename);
    if (dhdl)
      return dhdl;
  }
  
  /* ask for Content Directory Service (CDS) */
  if (!strcmp (filename, CDS_LOCATION))
    return http_get_file_from_memory (CDS_LOCATION,
                                      CDS_DESCRIPTION, CDS_DESCRIPTION_LEN);
  
  /* ask for Connection Manager Service (CMS) */
  if (!strcmp (filename, CMS_LOCATION))
    return http_get_file_from_memory (CMS_LOCATION,
                                      CMS_DESCRIPTION, CMS_DESCRIPTION_LEN);
  
  /* ask for AVTransport Service (AVTS) */
  if (!strcmp (filename, AVTS_LOCATION))
    return http_get_file_from_memory (AVTS_LOCATION,
                                      AVTS_DESCRIPTION, AVTS_DESCRIPTION_LEN);
  
  /* ask for anything else ... */
  id = atoi (strrchr (filename, '/') + 1);
  item = vfs_get_item_by_id (dlna, id);
  if (!item)
    return NULL;

  return http_get_file_local (item);
}

static int
upnp_http_read (void *cookie,
                dlnaWebFileHandle fh,
                char *buf,
                size_t buflen)
{
  dlna_t *dlna;
  dlna_http_file_handler_t *dhdl;
  http_file_handler_t *hdl;
  ssize_t len = -1;

  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external && dlna->http_callback && dlna->http_callback->read)
  {
    int res;
    res = dlna->http_callback->read (dhdl->priv, buf, buflen);
    if (res > 0)
      return res;
  }

  hdl = (http_file_handler_t *) dhdl->priv;
  
  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    dlna_log (dlna, DLNA_MSG_INFO, "Read local file.\n");
    len = read (hdl->detail.local.fd, buf, buflen);
    break;
  case HTTP_FILE_MEMORY:
    dlna_log (dlna, DLNA_MSG_INFO, "Read file from memory.\n");
    len = (ssize_t) FFMIN (buflen, hdl->detail.memory.len - hdl->pos);
    memcpy (buf, hdl->detail.memory.content + hdl->pos, (ssize_t) len);
    break;
  default:
    dlna_log (dlna, DLNA_MSG_ERROR, "Unknown HTTP file type.\n");
    break;
  }

  if (len > 0)
    hdl->pos += len;

  dlna_log (dlna, DLNA_MSG_INFO, "Read %zd bytes.\n", len);

  return len;
}

static int
upnp_http_write (void *cookie,
                 dlnaWebFileHandle fh,
                 char *buf,
                 size_t buflen)
{
  dlna_t *dlna;
  dlna_http_file_handler_t *dhdl;

  dlna = (dlna_t *) cookie;
  dhdl = (dlna_http_file_handler_t *) fh;
  
  /* trap application-level HTTP callback */
  if (dhdl->external && dlna->http_callback && dlna->http_callback->write)
  {
    int res;
    res = dlna->http_callback->write (dhdl->priv, buf, buflen);
    if (res > 0)
      return res;
  }
  
  return 0;
}

static int
upnp_http_seek (void *cookie,
                dlnaWebFileHandle fh,
                off_t offset,
                int origin)
{
  dlna_t *dlna;
  dlna_http_file_handler_t *dhdl;
  http_file_handler_t *hdl;
  off_t newpos = -1;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external && dlna->http_callback && dlna->http_callback->seek)
  {
    int res;
    res = dlna->http_callback->seek (dhdl->priv, offset, origin);
    if (res == 0)
      return res;
  }

  hdl = (http_file_handler_t *) dhdl->priv;
  
  switch (origin)
  {
  case SEEK_SET:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek to %lld (was at %lld) in %s\n",
              offset, hdl->pos, hdl->fullpath);
    newpos = offset;
    break;
  case SEEK_CUR:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek by %lld from %lld in %s\n",
              offset, hdl->pos, hdl->fullpath);
    newpos = hdl->pos + offset;
    break;
  case SEEK_END:
    dlna_log (dlna, DLNA_MSG_INFO,
              "Attempting to seek by %lld from end (was at %lld) in %s\n",
              offset, hdl->pos, hdl->fullpath);

    if (hdl->type == HTTP_FILE_LOCAL)
    {
      struct stat sb;
      if (stat (hdl->fullpath, &sb) < 0)
      {
        dlna_log (dlna, DLNA_MSG_ERROR,
                  "%s: cannot stat: %s\n", hdl->fullpath, strerror (errno));
        return HTTP_ERROR;
      }
      newpos = sb.st_size + offset;
    }
    else if (hdl->type == HTTP_FILE_MEMORY)
      newpos = hdl->detail.memory.len + offset;
    break;
  }

  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    /* Just make sure we cannot seek before start of file. */
    if (newpos < 0)
    {
      dlna_log (dlna, DLNA_MSG_ERROR,
                "%s: cannot seek: %s\n", hdl->fullpath, strerror (EINVAL));
      return HTTP_ERROR;
    }

    /* Don't seek with origin as specified above, as file may have
       changed in size since our last stat. */
    if (lseek (hdl->detail.local.fd, newpos, SEEK_SET) == -1)
    {
      dlna_log (dlna, DLNA_MSG_ERROR,
                "%s: cannot seek: %s\n", hdl->fullpath, strerror (errno));
      return HTTP_ERROR;
    }
    break;
  case HTTP_FILE_MEMORY:
    if (newpos < 0 || newpos > hdl->detail.memory.len)
    {
      dlna_log (dlna, DLNA_MSG_ERROR,
                "%s: cannot seek: %s\n", hdl->fullpath, strerror (EINVAL));
      return HTTP_ERROR;
    }
    break;
  }

  /* everything went well ... */
  hdl->pos = newpos;
  return HTTP_OK;
}

static int
upnp_http_close (void *cookie,
                 dlnaWebFileHandle fh)
{
  dlna_t *dlna;
  dlna_http_file_handler_t *dhdl;
  http_file_handler_t *hdl;
  
  if (!cookie || !fh)
    return HTTP_ERROR;

  dlna = (dlna_t *) cookie;
  dhdl = (dlna_http_file_handler_t *) fh;
  
  dlna_log (dlna, DLNA_MSG_INFO, "%s\n", __FUNCTION__);

  /* trap application-level HTTP callback */
  if (dhdl->external && dlna->http_callback && dlna->http_callback->close)
  {
    int res;
    res = dlna->http_callback->close (dhdl->priv);
    if (res == 0)
    {
      free (dhdl);
      return res;
    }
  }

  hdl = (http_file_handler_t *) dhdl->priv;
  
  switch (hdl->type)
  {
  case HTTP_FILE_LOCAL:
    close (hdl->detail.local.fd);
    break;
  case HTTP_FILE_MEMORY:
    /* no close operation is needed, just free file content */
    if (hdl->detail.memory.content)
      free (hdl->detail.memory.content);
    break;
  default:
    dlna_log (dlna, DLNA_MSG_ERROR, "Unknown HTTP file type.\n");
    break;
  }

  if (hdl->fullpath)
    free (hdl->fullpath);
  free (hdl);
  free (dhdl);

  return HTTP_OK;
}

struct dlnaVirtualDirCallbacks virtual_dir_callbacks = {
  NULL,
  upnp_http_get_info,
  upnp_http_open,
  upnp_http_read,
  upnp_http_write,
  upnp_http_seek,
  upnp_http_close
};
