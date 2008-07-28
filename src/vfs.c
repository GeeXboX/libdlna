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

#include <stdlib.h>
#include <limits.h>

#include "upnp_internals.h"

#define STARTING_ENTRY_ID_XBOX360 100000

void
vfs_item_free (dlna_t *dlna, vfs_item_t *item)
{
  if (!dlna || !dlna->vfs_root || !item)
    return;

  HASH_DEL (dlna->vfs_root, item);
  
  if (item->title)
    free (item->title);

  switch (item->type)
  {
  case DLNA_RESOURCE:
    if (item->u.resource.item)
      dlna_item_free (item->u.resource.item);
    if (item->u.resource.fullpath)
      free (item->u.resource.fullpath);
    if (item->u.resource.url)
      free (item->u.resource.url);
    break;
  case DLNA_CONTAINER:
  {
    vfs_item_t **children;
    for (children = item->u.container.children; *children; children++)
      vfs_item_free (dlna, *children);
    free (item->u.container.children);
    break;
  }
  }
  
  item->parent = NULL;
}

static dlna_status_code_t
vfs_is_id_registered (dlna_t *dlna, uint32_t id)
{
  vfs_item_t *item = NULL;

  if (!dlna || !dlna->vfs_root)
    return DLNA_ST_ERROR;

  HASH_FIND_INT (dlna->vfs_root, &id, item);

  return item ? DLNA_ST_OK : DLNA_ST_ERROR;
}

static uint32_t
vfs_provide_next_id (dlna_t *dlna)
{
  uint32_t i;
  uint32_t start = 1;

  if (dlna->mode == DLNA_CAPABILITY_UPNP_AV_XBOX)
    start += STARTING_ENTRY_ID_XBOX360;

  if (!dlna->vfs_root)
    return (start - 1);
  
  for (i = start; i < UINT_MAX; i++)
    if (vfs_is_id_registered (dlna, i) == DLNA_ST_ERROR)
      return i;

  return (start - 1);
}

vfs_item_t *
vfs_get_item_by_id (dlna_t *dlna, uint32_t id)
{
  vfs_item_t *item = NULL;

  if (!dlna || !dlna->vfs_root)
    return NULL;

  HASH_FIND_INT (dlna->vfs_root, &id, item);

  return item;
}

vfs_item_t *
vfs_get_item_by_name (dlna_t *dlna, char *name)
{
  vfs_item_t *item = NULL;

  if (!dlna || !dlna->vfs_root)
    return NULL;
  
  for (item = dlna->vfs_root; item; item = item->hh.next)
    if (!strcmp (item->title, name))
      return item;

  return NULL;
}

static int
list_get_length (void *list)
{
  void **l = list;
  int n = 0;

  while (*(l++))
    n++;

  return n;
}

static void
vfs_item_add_child (dlna_t *dlna, vfs_item_t *item, vfs_item_t *child)
{
  vfs_item_t **children;
  int n;

  if (!dlna || !item || !child)
    return;

  for (children = item->u.container.children; *children; children++)
    if (*children == child)
      return; /* already present */

  n = list_get_length ((void *) item->u.container.children) + 1;
  item->u.container.children = (vfs_item_t **)
    realloc (item->u.container.children,
             (n + 1) * sizeof (*(item->u.container.children)));
  item->u.container.children[n] = NULL;
  item->u.container.children[n - 1] = child;
  item->u.container.children_count++;
  dlna->vfs_items++;
}

uint32_t
dlna_vfs_add_container (dlna_t *dlna, char *name,
                        uint32_t object_id, uint32_t container_id)
{
  vfs_item_t *item;
  
  if (!dlna || !name)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "Adding container '%s'\n", name);
  
  item = calloc (1, sizeof (vfs_item_t));

  item->type = DLNA_CONTAINER;
  
  /* is requested 'object_id' available ? */
  if (object_id == 0 || vfs_is_id_registered (dlna, object_id) == DLNA_ST_OK)
    item->id = vfs_provide_next_id (dlna);
  else
    item->id = object_id;

  HASH_ADD_INT (dlna->vfs_root, id, item);
  
  dlna_log (dlna, DLNA_MSG_INFO,
            "New container id (asked for #%d, granted #%d)\n",
            object_id, item->id);

  item->title = strdup (name);

  item->u.container.children = calloc (1, sizeof (vfs_item_t *));
  *(item->u.container.children) = NULL;
  item->u.container.children_count = 0;
  
  if (!dlna->vfs_root)
    dlna->vfs_root = item;
  
  /* check for a valid parent id */
  item->parent = vfs_get_item_by_id (dlna, container_id);
  if (!item->parent)
    item->parent = dlna->vfs_root;

  /* add new child to parent */
  if (item->parent != item)
    vfs_item_add_child (dlna, item->parent, item);

  dlna_log (dlna, DLNA_MSG_INFO, "Container is parent of #%d (%s)\n",
            item->parent->id, item->parent->title);
  
  return item->id;
}

uint32_t
dlna_vfs_add_resource (dlna_t *dlna, char *name,
                       char *fullpath, off_t size, uint32_t container_id)
{
  vfs_item_t *item, *parent;
  
  if (!dlna || !name || !fullpath)
    return 0;

  if (!dlna->vfs_root)
  {
    dlna_log (dlna, DLNA_MSG_ERROR, "No VFS root found. Add one first\n");
    return 0;
  }

  item = calloc (1, sizeof (vfs_item_t));

  item->type = DLNA_RESOURCE;
  
  item->id = vfs_provide_next_id (dlna);
  item->title = strdup (name);

  item->u.resource.item = dlna_item_new (dlna, fullpath);
  item->u.resource.cnv = DLNA_ORG_CONVERSION_NONE;

  HASH_ADD_INT (dlna->vfs_root, id, item);
  
  if (!item->u.resource.item)
  {
    dlna_log (dlna, DLNA_MSG_WARNING,
              "Specified resource is not DLNA compliant. "
              "Transcoding is needed (but not yet supported)\n");
    vfs_item_free (dlna, item);
    return 0;
  }

  dlna_log (dlna, DLNA_MSG_INFO, "New resource id #%d (%s)\n",
            item->id, item->title);
  item->u.resource.fullpath = strdup (fullpath);
  item->u.resource.size = size;
  item->u.resource.fd = -1;
  
  /* determine parent */
  parent = vfs_get_item_by_id (dlna, container_id);
  item->parent = parent ? parent : dlna->vfs_root;

  dlna_log (dlna, DLNA_MSG_INFO,
            "Resource is parent of #%d (%s)\n", parent->id, parent->title);

  /* add new child to parent */
  vfs_item_add_child (dlna, item->parent, item);
  
  return item->id;
}

void
dlna_vfs_remove_item_by_id (dlna_t *dlna, uint32_t id)
{
  vfs_item_t *item;

  if (!dlna)
    return;
  
  item = vfs_get_item_by_id (dlna, id);
  dlna_log (dlna, DLNA_MSG_INFO,
            "Removing item #%d (%s)\n", item->id, item->title);
  vfs_item_free (dlna, item);
}

void
dlna_vfs_remove_item_by_title (dlna_t *dlna, char *name)
{
  vfs_item_t *item;

  if (!dlna || !name)
    return;

  item = vfs_get_item_by_name (dlna, name);
  dlna_log (dlna, DLNA_MSG_INFO,
            "Removing item #%d (%s)\n", item->id, item->title);
  vfs_item_free (dlna, item);
}
