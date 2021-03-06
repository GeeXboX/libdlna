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

/*
 * ContentDirectory service specifications can be found at:
 * http://upnp.org/standardizeddcps/documents/ContentDirectory1.0.pdf
 * http://upnp.org/specs/av/UPnP-av-ContentDirectory-v2-Service-20060531.pdf
 */

#include <stdlib.h>

#include "upnp_internals.h"
#include "minmax.h"

/* CDS Action Names */
#define SERVICE_CDS_ACTION_SEARCH_CAPS        "GetSearchCapabilities"
#define SERVICE_CDS_ACTION_SORT_CAPS          "GetSortCapabilities"
#define SERVICE_CDS_ACTION_UPDATE_ID          "GetSystemUpdateID"
#define SERVICE_CDS_ACTION_BROWSE             "Browse"
#define SERVICE_CDS_ACTION_SEARCH             "Search"
#define SERVICE_CDS_ACTION_CREATE_OBJ         "CreateObject"
#define SERVICE_CDS_ACTION_DESTROY_OBJ        "DestroyObject"
#define SERVICE_CDS_ACTION_UPDATE_OBJ         "UpdateObject"
#define SERVICE_CDS_ACTION_IMPORT_RES         "ImportResource"
#define SERVICE_CDS_ACTION_EXPORT_RES         "ExportResource"
#define SERVICE_CDS_ACTION_STOP_TRANSFER      "StopTransferResource"
#define SERVICE_CDS_ACTION_GET_PROGRESS       "GetTransferProgress"
#define SERVICE_CDS_ACTION_DELETE_RES         "DeleteResource"
#define SERVICE_CDS_ACTION_CREATE_REF         "CreateReference"

/* CDS Arguments */
#define SERVICE_CDS_ARG_SEARCH_CAPS           "SearchCaps"
#define SERVICE_CDS_ARG_SORT_CAPS             "SortCaps"
#define SERVICE_CDS_ARG_UPDATE_ID             "Id"

/* CDS Browse Input Arguments */
#define SERVICE_CDS_ARG_OBJECT_ID             "ObjectID"
#define SERVICE_CDS_ARG_BROWSE_FLAG           "BrowseFlag"
#define SERVICE_CDS_ARG_FILTER                "Filter"
#define SERVICE_CDS_ARG_START_INDEX           "StartingIndex"
#define SERVICE_CDS_ARG_REQUEST_COUNT         "RequestedCount"
#define SERVICE_CDS_ARG_SORT_CRIT             "SortCriteria"
#define SERVICE_CDS_ARG_SEARCH_CRIT           "SearchCriteria"

/* CDS Argument Values */
#define SERVICE_CDS_ROOT_OBJECT_ID            "0"
#define SERVICE_CDS_BROWSE_METADATA           "BrowseMetadata"
#define SERVICE_CDS_BROWSE_CHILDREN           "BrowseDirectChildren"
#define SERVICE_CDS_OBJECT_CONTAINER          "object.container.storageFolder"
#define SERVICE_CDS_DIDL_RESULT               "Result"
#define SERVICE_CDS_DIDL_NUM_RETURNED         "NumberReturned"
#define SERVICE_CDS_DIDL_TOTAL_MATCH          "TotalMatches"
#define SERVICE_CDS_DIDL_UPDATE_ID            "UpdateID"

/* CDS Search Parameters */
#define SEARCH_CLASS_MATCH_KEYWORD            "(upnp:class = \""
#define SEARCH_CLASS_DERIVED_KEYWORD          "(upnp:class derivedfrom \""
#define SEARCH_PROTOCOL_CONTAINS_KEYWORD      "(res@protocolInfo contains \""
#define SEARCH_OBJECT_KEYWORD                 "object"
#define SEARCH_AND                            ") and ("

/* CDS DIDL Messages */
#define DIDL_NAMESPACE \
    "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" " \
    "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " \
    "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\""

#define DIDL_LITE                             "DIDL-Lite"
#define DIDL_ITEM                             "item"
#define DIDL_ITEM_ID                          "id"
#define DIDL_ITEM_PARENT_ID                   "parentID"
#define DIDL_ITEM_RESTRICTED                  "restricted"
#define DIDL_ITEM_CLASS                       "upnp:class"
#define DIDL_ITEM_TITLE                       "dc:title"
#define DIDL_ITEM_ARTIST                      "dc:artist"
#define DIDL_ITEM_DESCRIPTION                 "dc:description"
#define DIDL_ITEM_ALBUM                       "upnp:album"
#define DIDL_ITEM_TRACK                       "upnp:originalTrackNumber"
#define DIDL_ITEM_GENRE                       "upnp:genre"
#define DIDL_RES                              "res"
#define DIDL_RES_INFO                         "protocolInfo"
#define DIDL_RES_SIZE                         "size"
#define DIDL_RES_DURATION                     "duration"
#define DIDL_RES_BITRATE                      "bitrate"
#define DIDL_RES_SAMPLE_FREQUENCY             "sampleFrequency"
#define DIDL_RES_BPS                          "bitsPerSample"
#define DIDL_RES_AUDIO_CHANNELS               "nrAudioChannels"
#define DIDL_RES_RESOLUTION                   "resolution"
#define DIDL_CONTAINER                        "container"
#define DIDL_CONTAINER_ID                     "id"
#define DIDL_CONTAINER_PARENT_ID              "parentID"
#define DIDL_CONTAINER_CHILD_COUNT            "childCount"
#define DIDL_CONTAINER_RESTRICTED             "restricted"
#define DIDL_CONTAINER_SEARCH                 "searchable"
#define DIDL_CONTAINER_CLASS                  "upnp:class"
#define DIDL_CONTAINER_TITLE                  "dc:title"

/* CDS Error Codes */
#define CDS_ERR_INVALID_ACTION                401
#define CDS_ERR_INVALID_ARGS                  402
#define CDS_ERR_INVALID_VAR                   404
#define CDS_ERR_ACTION_FAILED                 501
#define CDS_ERR_INVALID_OBJECT_ID             701
#define CDS_ERR_INVALID_CURRENT_TAG_VALUE     702
#define CDS_ERR_INVALID_NEW_TAG_VALUE         703
#define CDS_ERR_REQUIRED_TAG                  704
#define CDS_ERR_READ_ONLY_TAG                 705
#define CDS_ERR_PARAMETER_MISMATCH            706
#define CDS_ERR_INVALID_SEARCH_CRITERIA       708
#define CDS_ERR_INVALID_SORT_CRITERIA         709
#define CDS_ERR_INVALID_CONTAINER             710
#define CDS_ERR_RESTRICTED_OBJECT             711
#define CDS_ERR_BAD_METADATA                  712
#define CDS_ERR_RESTRICTED_PARENT             713
#define CDS_ERR_INVALID_SOURCE                714
#define CDS_ERR_ACCESS_DENIED_SOURCE          715
#define CDS_ERR_TRANSFER_BUSY                 716
#define CDS_ERR_INVALID_TRANSFER_ID           717
#define CDS_ERR_INVALID_DESTINATION           718
#define CDS_ERR_ACESS_DENIED_DESTINATION      719
#define CDS_ERR_PROCESS_REQUEST               720

/*
 * GetSearchCapabilities:
 *   This action returns the searching capabilities that
 *   are supported by the device.
 */
static int
cds_get_search_capabilities (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  upnp_add_response (ev, SERVICE_CDS_ARG_SEARCH_CAPS, "");
  
  return ev->status;
}

/*
 * GetSortCapabilities:
 *   Returns the CSV list of meta-data tags that can be used in sortCriteria.
 */
static int
cds_get_sort_capabilities (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  upnp_add_response (ev, SERVICE_CDS_ARG_SORT_CAPS, "");
  
  return ev->status;
}

/*
 * GetSystemUpdateID:
 *   This action returns the current value of state variable SystemUpdateID.
 *   It can be used by clients that want to poll for any changes in
 *   the Content Directory (as opposed to subscribing to events).
 */
static int
cds_get_system_update_id (dlna_t *dlna, upnp_action_event_t *ev)
{
  if (!dlna || !ev)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  upnp_add_response (ev, SERVICE_CDS_ARG_UPDATE_ID,
                     SERVICE_CDS_ROOT_OBJECT_ID);
  
  return ev->status;
}

static int
filter_has_val (const char *filter, const char *val)
{
  char *x = NULL, *token = NULL;
  char *m_buffer = NULL, *buffer;
  int len = strlen (val);
  int ret = 0;

  if (!strcmp (filter, "*"))
    return 1;

  x = strdup (filter);
  if (!x)
    return 0;

  m_buffer = malloc (strlen (x));
  if (!m_buffer)
  {
    free (x);
    return 0;
  }

  buffer = m_buffer;
  token = strtok_r (x, ",", &buffer);
  while (token)
  {
    if (*val == '@')
      token = strchr (token, '@');
    if (token && !strncmp (token, val, len))
    {
      ret = 1;
      break;
    }
    token = strtok_r (NULL, ",", &buffer);
  }

  free (m_buffer);
  free (x);
  
  return ret;
}

static void
didl_add_header (buffer_t *out)
{
  buffer_appendf (out, "<%s %s>", DIDL_LITE, DIDL_NAMESPACE);
}

static void
didl_add_footer (buffer_t *out)
{
  buffer_appendf (out, "</%s>", DIDL_LITE);
}

static void
didl_add_tag (buffer_t *out, char *tag, char *value)
{
  if (value)
    buffer_appendf (out, "<%s>%s</%s>", tag, value, tag);
}

static void
didl_add_param (buffer_t *out, char *param, char *value)
{
  if (value)
    buffer_appendf (out, " %s=\"%s\"", param, value);
}

static void
didl_add_value (buffer_t *out, char *param, off_t value)
{
  buffer_appendf (out, " %s=\"%lld\"", param, value);
}

static void
didl_add_item (dlna_t *dlna, buffer_t *out, vfs_item_t *item,
               char *restricted, char *filter)
{

  char *class;
     
  buffer_appendf (out, "<%s", DIDL_ITEM);
  didl_add_value (out, DIDL_ITEM_ID, item->id);
  didl_add_value (out, DIDL_ITEM_PARENT_ID,
                  item->parent ? item->parent->id : 0);
  didl_add_param (out, DIDL_ITEM_RESTRICTED, restricted);
  buffer_append (out, ">");

  class = dlna_profile_upnp_object_item (item->u.resource.item->profile);

  if (item->u.resource.item->metadata &&
      strlen (item->u.resource.item->metadata->title) > 1)
    didl_add_tag (out, DIDL_ITEM_TITLE,
                  item->u.resource.item->metadata->title);
  else
    didl_add_tag (out, DIDL_ITEM_TITLE, item->title);
  
  didl_add_tag (out, DIDL_ITEM_CLASS, class);

  if (item->u.resource.item->metadata)
  {
    if (strlen (item->u.resource.item->metadata->author) > 1)
      didl_add_tag (out, DIDL_ITEM_ARTIST,
                    item->u.resource.item->metadata->author);
    if (strlen (item->u.resource.item->metadata->comment) > 1)
      didl_add_tag (out, DIDL_ITEM_DESCRIPTION,
                    item->u.resource.item->metadata->comment);
    if (strlen (item->u.resource.item->metadata->album) > 1)
      didl_add_tag (out, DIDL_ITEM_ALBUM,
                    item->u.resource.item->metadata->album);
    if (item->u.resource.item->metadata->track)
      didl_add_value (out, DIDL_ITEM_TRACK,
                      item->u.resource.item->metadata->track);
    if (strlen (item->u.resource.item->metadata->genre) > 1)
      didl_add_tag (out, DIDL_ITEM_GENRE,
                    item->u.resource.item->metadata->genre);
  }
  
  if (filter_has_val (filter, DIDL_RES))
  {
    char *protocol_info;

    protocol_info =
      dlna_write_protocol_info (DLNA_PROTOCOL_INFO_TYPE_HTTP,
                                DLNA_ORG_PLAY_SPEED_NORMAL,
                                item->u.resource.cnv,
                                DLNA_ORG_OPERATION_RANGE,
                                dlna->flags, item->u.resource.item->profile);
    
    buffer_appendf (out, "<%s", DIDL_RES);
    didl_add_param (out, DIDL_RES_INFO, protocol_info);
    free (protocol_info);
    
    if (filter_has_val (filter, "@"DIDL_RES_SIZE))
      didl_add_value (out, DIDL_RES_SIZE, item->u.resource.size);
    
    didl_add_param (out, DIDL_RES_DURATION,
                    item->u.resource.item->properties->duration);
    didl_add_value (out, DIDL_RES_BITRATE,
                    item->u.resource.item->properties->bitrate);
    didl_add_value (out, DIDL_RES_BPS,
                    item->u.resource.item->properties->bps);
    didl_add_value (out, DIDL_RES_AUDIO_CHANNELS,
                    item->u.resource.item->properties->channels);
    if (strlen (item->u.resource.item->properties->resolution) > 1)
      didl_add_param (out, DIDL_RES_RESOLUTION,
                      item->u.resource.item->properties->resolution);

    buffer_append (out, ">");
    buffer_appendf (out, "http://%s:%d%s/%d",
                    dlnaGetServerIpAddress (),
                    dlna->port, VIRTUAL_DIR, item->id);
    buffer_appendf (out, "</%s>", DIDL_RES);
  }
  buffer_appendf (out, "</%s>", DIDL_ITEM);
}

static void
didl_add_container (buffer_t *out, vfs_item_t *item,
                    char *restricted, char *searchable)
{
  buffer_appendf (out, "<%s", DIDL_CONTAINER);

  didl_add_value (out, DIDL_CONTAINER_ID, item->id);
  didl_add_value (out, DIDL_CONTAINER_PARENT_ID,
                  item->parent ? item->parent->id : 0);
  
  didl_add_value (out, DIDL_CONTAINER_CHILD_COUNT,
                  item->u.container.children_count);
  
  didl_add_param (out, DIDL_CONTAINER_RESTRICTED, restricted);
  didl_add_param (out, DIDL_CONTAINER_SEARCH, searchable);
  buffer_append (out, ">");

  didl_add_tag (out, DIDL_CONTAINER_CLASS, SERVICE_CDS_OBJECT_CONTAINER);
  didl_add_tag (out, DIDL_CONTAINER_TITLE, item->title);

  buffer_appendf (out, "</%s>", DIDL_CONTAINER);
}

static int
cds_browse_metadata (dlna_t *dlna, upnp_action_event_t *ev,
                     buffer_t *out, vfs_item_t *item, char *filter)
{
  int result_count = 0;

  if (!item)
    return -1;

  didl_add_header (out);
  switch (item->type)
  {
  case DLNA_RESOURCE:
    didl_add_item (dlna, out, item, "false", filter);
    break;

  case DLNA_CONTAINER:
    didl_add_container (out, item, "true", "true");
    result_count = 1;
    break;

  default:
    break;
  }
  didl_add_footer (out);
  
  upnp_add_response (ev, SERVICE_CDS_DIDL_RESULT, out->buf);
  upnp_add_response (ev, SERVICE_CDS_DIDL_NUM_RETURNED, "1");
  upnp_add_response (ev, SERVICE_CDS_DIDL_TOTAL_MATCH, "1");

  return result_count;
}

static int
cds_browse_directchildren (dlna_t *dlna, upnp_action_event_t *ev,
                           buffer_t *out, int index,
                           int count, vfs_item_t *item, char *filter)
{
  vfs_item_t **items;
  int s, result_count = 0;
  char tmp[32];

  /* browsing direct children only has a sense on containers */
  if (item->type != DLNA_CONTAINER)
    return -1;
  
  didl_add_header (out);

  /* go to the child pointed out by index */
  items = item->u.container.children;
  for (s = 0; s < index; s++)
    if (*items)
      items++;

  /* UPnP CDS compliance : If starting index = 0 and requested count = 0
     then all children must be returned */
  if (index == 0 && count == 0)
    count = item->u.container.children_count;

  for (; *items; items++)
  {
    /* only fetch the requested count number or all entries if count = 0 */
    if (count == 0 || result_count < count)
    {
      switch ((*items)->type)
      {
      case DLNA_CONTAINER:
        didl_add_container (out, *items, "true", NULL);
        break;

      case DLNA_RESOURCE:
        didl_add_item (dlna, out, *items, "true", filter);
        break;

      default:
        break;
      }
      result_count++;
    }
  }

  didl_add_footer (out);

  upnp_add_response (ev, SERVICE_CDS_DIDL_RESULT, out->buf);
  sprintf (tmp, "%d", result_count);
  upnp_add_response (ev, SERVICE_CDS_DIDL_NUM_RETURNED, tmp);
  sprintf (tmp, "%d", item->u.container.children_count);
  upnp_add_response (ev, SERVICE_CDS_DIDL_TOTAL_MATCH, tmp);

  return result_count;
}

/*
 * Browse:
 *   This action allows the caller to incrementally browse the native
 *   hierarchy of the Content Directory objects exposed by the Content
 *   Directory Service, including information listing the classes of objects
 *   available in any particular object container.
 */
static int
cds_browse (dlna_t *dlna, upnp_action_event_t *ev)
{
  /* input arguments */
  int id, index, count, sort;
  char *flag = NULL, *filter = NULL;

  /* output arguments */
  buffer_t *out = NULL;
  int result_count = 0;
  vfs_item_t *item;
  int meta;
  
  if (!dlna || !ev)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  /* Check for status */
  if (!ev->status)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }
  
  /* Retrieve input arguments */
  id     = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_OBJECT_ID);
  flag   = upnp_get_string (ev->ar, SERVICE_CDS_ARG_BROWSE_FLAG);
  filter = upnp_get_string (ev->ar, SERVICE_CDS_ARG_FILTER);
  index  = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_START_INDEX);
  count  = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_REQUEST_COUNT);
  sort   = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_SORT_CRIT);

  if (!flag || !filter)
  {
    ev->ar->ErrCode = CDS_ERR_INVALID_ARGS;
    goto browse_err;
  }
 
  /* check for arguments validity */
  if (!strcmp (flag, SERVICE_CDS_BROWSE_METADATA))
  {
    if (index)
    {
      ev->ar->ErrCode = CDS_ERR_PROCESS_REQUEST;
      goto browse_err;
    }
    meta = 1;
    }
  else if (!strcmp (flag, SERVICE_CDS_BROWSE_CHILDREN))
    meta = 0;
  else
  {
    ev->ar->ErrCode = CDS_ERR_PROCESS_REQUEST;
    goto browse_err;
  }
  free (flag);

  /* find requested item in VFS */
  item = vfs_get_item_by_id (dlna, id);
  if (!item)
    item = vfs_get_item_by_id (dlna, 0);

  if (!item)
  {
    ev->ar->ErrCode = CDS_ERR_INVALID_OBJECT_ID;
    goto browse_err;
  }

  out = buffer_new ();
  result_count = meta ?
    cds_browse_metadata (dlna, ev, out, item, filter) :
    cds_browse_directchildren (dlna, ev, out, index, count, item, filter);
  
  free (filter);

  if (result_count < 0)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    goto browse_err;
  }

  buffer_free (out);
  upnp_add_response (ev, SERVICE_CDS_DIDL_UPDATE_ID,
                     SERVICE_CDS_ROOT_OBJECT_ID);
  
  return ev->status;

 browse_err:
  if (flag)
    free (flag);
  if (filter)
    free (filter);
  if (out)
    buffer_free (out);

  return 0;
}

static int
cds_search_match (dlna_t *dlna, vfs_item_t *item, char *search_criteria)
{
  char keyword[256];
  int derived_from = 0, protocol_contains = 0, result = 0;
  char *and_clause = NULL;
  char *protocol_info;
  char *object_type = NULL;
  
  if (!item || !search_criteria)
    return 0;

  memset (keyword, '\0', sizeof (keyword));
  
  if (!strncmp (search_criteria, SEARCH_CLASS_MATCH_KEYWORD,
                strlen (SEARCH_CLASS_MATCH_KEYWORD)))
  {
    strncpy (keyword, search_criteria
             + strlen (SEARCH_CLASS_MATCH_KEYWORD), sizeof (keyword));
  }
  else if (!strncmp (search_criteria, SEARCH_CLASS_DERIVED_KEYWORD,
                     strlen (SEARCH_CLASS_DERIVED_KEYWORD)))
  {
    derived_from = 1;
    strncpy (keyword, search_criteria
             + strlen (SEARCH_CLASS_DERIVED_KEYWORD), sizeof (keyword));
  }
  else if (!strncmp (search_criteria, SEARCH_PROTOCOL_CONTAINS_KEYWORD,
                     strlen (SEARCH_PROTOCOL_CONTAINS_KEYWORD)))
  {
    protocol_contains = 1;
    strncpy (keyword, search_criteria
             + strlen (SEARCH_PROTOCOL_CONTAINS_KEYWORD), sizeof (keyword));
  }
  else
    strcpy (keyword, SEARCH_OBJECT_KEYWORD);

  protocol_info =
    dlna_write_protocol_info (DLNA_PROTOCOL_INFO_TYPE_HTTP,
                              DLNA_ORG_PLAY_SPEED_NORMAL,
                              item->u.resource.cnv,
                              DLNA_ORG_OPERATION_RANGE,
                              dlna->flags, item->u.resource.item->profile);

  object_type = dlna_profile_upnp_object_item (item->u.resource.item->profile);
  
  if (derived_from && object_type
      && !strncmp (object_type, keyword, strlen (keyword)))
    result = 1;
  else if (protocol_contains && strstr (protocol_info, keyword))
    result = 1;
  else if (object_type && !strcmp (object_type, keyword))
    result = 1;
  free (protocol_info);
  
  and_clause = strstr (search_criteria, SEARCH_AND);
  if (and_clause)
    return (result && cds_search_match (dlna, item,
                                        and_clause + strlen (SEARCH_AND) -1));

  return 1;
}

static int
cds_search_recursive (dlna_t *dlna, vfs_item_t *item, buffer_t *out,
                      int count, char *filter, char *search_criteria)
{
  vfs_item_t **items;
  int result_count = 0;

  if (item->type != DLNA_CONTAINER)
    return 0;
  
  /* go to the first child */
  items = item->u.container.children;
  
  for (; *items; items++)
  {
    /* only fetch the requested count number or all entries if count = 0 */
    if (count == 0 || result_count < count)
    {
      switch ((*items)->type)
      {
      case DLNA_CONTAINER:
        result_count +=
          cds_search_recursive (dlna, *items, out,
                                (count == 0) ? 0 : (count - result_count),
                                filter, search_criteria);
        break;

      case DLNA_RESOURCE:        
        if (cds_search_match (dlna, *items, search_criteria))
        {
          didl_add_item (dlna, out, *items, "true", filter);
          result_count++;
        }
        break;

      default:
        break;
      }
    }
  }

  return result_count;
}

static int
cds_search_directchildren (dlna_t *dlna, upnp_action_event_t *ev,
                           vfs_item_t *item, buffer_t *out, int index,
                           int count, char *filter, char *search_criteria)
{
  vfs_item_t **items;
  int i, result_count = 0;
  char tmp[32];
  
  index = 0;

  /* searching only has a sense on containers */
  if (item->type != DLNA_CONTAINER)
    return -1;
  
  didl_add_header (out);

  /* go to the child pointed out by index */
  items = item->u.container.children;
  for (i = 0; i < index; i++)
    if (*items)
      items++;

  /* UPnP CDS compliance : If starting index = 0 and requested count = 0
     then all children must be returned */
  if (index == 0 && count == 0)
    count = item->u.container.children_count;

  result_count =
    cds_search_recursive (dlna, item, out, count, filter, search_criteria);

  didl_add_footer (out);

  upnp_add_response (ev, SERVICE_CDS_DIDL_RESULT, out->buf);
  sprintf (tmp, "%d", result_count);
  upnp_add_response (ev, SERVICE_CDS_DIDL_NUM_RETURNED, tmp);
  sprintf (tmp, "%d", result_count);
  upnp_add_response (ev, SERVICE_CDS_DIDL_TOTAL_MATCH, tmp);

  return result_count;
}

/*
 * Search:
 *   This action allows the caller to search the content directory for
 *   objects that match some search criteria.
 *   The search criteria are specified as a query string operating on
 *   properties with comparison and logical operators.
 */
static int
cds_search (dlna_t *dlna, upnp_action_event_t *ev)
{
  /* input arguments */
  int index, count, id, sort_criteria;
  char *search_criteria = NULL, *filter = NULL;

  /* output arguments */
  buffer_t *out = NULL;
  vfs_item_t *item;
  int result_count = 0;
  
  if (!dlna || !ev)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  /* Check for status */
  if (!ev->status)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    return 0;
  }

  /* Retrieve input arguments */
  id              = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_OBJECT_ID);
  search_criteria = upnp_get_string (ev->ar,
                                     SERVICE_CDS_ARG_SEARCH_CRIT);
  filter          = upnp_get_string (ev->ar, SERVICE_CDS_ARG_FILTER);
  index           = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_START_INDEX);
  count           = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_REQUEST_COUNT);
  sort_criteria   = upnp_get_ui4    (ev->ar, SERVICE_CDS_ARG_SORT_CRIT);

  if (!search_criteria || !filter)
  {
    ev->ar->ErrCode = CDS_ERR_INVALID_ARGS;
    goto search_err;
  }

  /* find requested item in VFS */
  item = vfs_get_item_by_id (dlna, id);
  if (!item)
    item = vfs_get_item_by_id (dlna, 0);

  if (!item)
  {
    ev->ar->ErrCode = CDS_ERR_INVALID_CONTAINER;
    goto search_err;
  }
  
  out = buffer_new ();
  result_count = cds_search_directchildren (dlna, ev, item, out, index, count,
                                            filter, search_criteria);

  if (result_count < 0)
  {
    ev->ar->ErrCode = CDS_ERR_ACTION_FAILED;
    goto search_err;
  }
  
  buffer_free (out);
  upnp_add_response (ev, SERVICE_CDS_DIDL_UPDATE_ID,
                     SERVICE_CDS_ROOT_OBJECT_ID);

  free (search_criteria);
  free (filter);

  return ev->status;

 search_err:
  if (search_criteria)
    free (search_criteria);
  if (filter)
    free (filter);
  if (out)
    buffer_free (out);

  return 0;
}

/* List of UPnP ContentDirectory Service actions */
upnp_service_action_t cds_service_actions[] = {
  /* CDS Required Actions */
  { SERVICE_CDS_ACTION_SEARCH_CAPS,    cds_get_search_capabilities },
  { SERVICE_CDS_ACTION_SORT_CAPS,      cds_get_sort_capabilities },
  { SERVICE_CDS_ACTION_UPDATE_ID,      cds_get_system_update_id },
  { SERVICE_CDS_ACTION_BROWSE,         cds_browse },

  /* CDS Optional Actions */
  { SERVICE_CDS_ACTION_SEARCH,         cds_search },
  { SERVICE_CDS_ACTION_CREATE_OBJ,     NULL },
  { SERVICE_CDS_ACTION_DESTROY_OBJ,    NULL },
  { SERVICE_CDS_ACTION_UPDATE_OBJ,     NULL },
  { SERVICE_CDS_ACTION_IMPORT_RES,     NULL },
  { SERVICE_CDS_ACTION_EXPORT_RES,     NULL },
  { SERVICE_CDS_ACTION_STOP_TRANSFER,  NULL },
  { SERVICE_CDS_ACTION_GET_PROGRESS,   NULL },
  { SERVICE_CDS_ACTION_DELETE_RES,     NULL },
  { SERVICE_CDS_ACTION_CREATE_REF,     NULL },

  /* CDS Vendor-specific Actions */ 
  { NULL,                              NULL }
};
