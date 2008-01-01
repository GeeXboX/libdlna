/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007 Benjamin Zores <ben@geexbox.org>
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

void
vfs_item_free (vfs_item_t *item)
{
  if (!item)
    return;

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
      vfs_item_free (*children);
    free (item->u.container.children);
    break;
  }
  }
  
  item->parent = NULL;
}

static dlna_status_code_t
vfs_item_is_id_registered (vfs_item_t *item, uint32_t id)
{
  vfs_item_t **children;

  if (!item)
    return DLNA_ST_ERROR;

  /* matching 'id' */
  if (item->id == id)
    return DLNA_ST_OK;

  if (item->type != DLNA_CONTAINER)
    return DLNA_ST_ERROR;

  for (children = item->u.container.children; *children; children++)
    if (vfs_item_is_id_registered (*children, id) == DLNA_ST_OK)
      return DLNA_ST_OK;

  return DLNA_ST_ERROR;
}

static dlna_status_code_t
vfs_is_id_registered (dlna_t *dlna, uint32_t id)
{
  if (!dlna)
    return DLNA_ST_ERROR;

  return vfs_item_is_id_registered (dlna->vfs_root, id);
}

static uint32_t
vfs_provide_next_id (dlna_t *dlna)
{
  uint32_t i;

  if (!dlna->vfs_root)
    return 0;
  
  for (i = 1; i < UINT_MAX; i++)
    if (vfs_is_id_registered (dlna, i) == DLNA_ST_ERROR)
      return i;

  return 0;
}

static vfs_item_t *
vfs_get_item_by_id (vfs_item_t *item, uint32_t id)
{
  vfs_item_t **children;

  if (!item)
    return NULL;

  /* matching 'id' */
  if (item->id == id)
    return item;

  if (item->type != DLNA_CONTAINER)
    return NULL;

  for (children = item->u.container.children; *children; children++)
  {
    vfs_item_t *it;
    it = vfs_get_item_by_id (*children, id);
    if (it)
      return it;
  }

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
                        uint32_t id, uint32_t container_id)
{
  vfs_item_t *item;
  
  if (!dlna || !name)
    return 0;

  dlna_log (dlna, DLNA_MSG_INFO, "Adding container '%s'\n", name);
  
  item = calloc (1, sizeof (vfs_item_t));

  item->type = DLNA_CONTAINER;
  
  /* is requested 'id' available ? */
  if (id == 0 || vfs_is_id_registered (dlna, id) == DLNA_ST_OK)
    item->id = vfs_provide_next_id (dlna);
  else
    item->id = id;

  dlna_log (dlna, DLNA_MSG_INFO,
            "New container id (asked for #%d, granted #%d)\n", id, item->id);

  item->title = strdup (name);

  item->u.container.children = calloc (1, sizeof (vfs_item_t *));
  *(item->u.container.children) = NULL;
  item->u.container.children_count = 0;
  
  if (!dlna->vfs_root)
    dlna->vfs_root = item;
  
  /* check for a valid parent id */
  item->parent = vfs_get_item_by_id (dlna->vfs_root, container_id);
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

  if (!item->u.resource.item)
  {
    dlna_log (dlna, DLNA_MSG_WARNING,
              "Specified resource is not DLNA compliant. "
              "Transcoding is needed (but not yet supported)\n");
    vfs_item_free (item);
    return 0;
  }

  dlna_log (dlna, DLNA_MSG_INFO, "New resource id #%d\n", item->id);
  item->u.resource.fullpath = strdup (fullpath);
  item->u.resource.size = size;
  item->u.resource.fd = -1;
  
  /* determine parent */
  parent = vfs_get_item_by_id (dlna->vfs_root, container_id);
  item->parent = parent ? parent : dlna->vfs_root;

  dlna_log (dlna, DLNA_MSG_INFO,
            "Resource is parent of #%d (%s)\n", parent->id, parent->title);

  /* add new child to parent */
  vfs_item_add_child (dlna, item->parent, item);
  
  return item->id;
}
