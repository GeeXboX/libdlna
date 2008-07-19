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
#include <string.h>

#include "dlna_internals.h"
#include "profiles.h"
#include "containers.h"

typedef struct mime_type_s {
  const char *extension;
  const char *mime;
} mime_type_t;

static const mime_type_t mime_type_list[] = {
  /* Video files */
  { "asf",   "video/x-ms-asf"},
  { "avc",   "video/avi"},
  { "avi",   "video/avi"},
  { "dv",    "video/x-dv"},
  { "divx",  "video/avi"},
  { "wmv",   "video/x-ms-wmv"},
  { "mjpg",  "video/x-motion-jpeg"},
  { "mjpeg", "video/x-motion-jpeg"},
  { "mpeg",  "video/mpeg"},
  { "mpg",   "video/mpeg"},
  { "mpe",   "video/mpeg"},
  { "mp2p",  "video/mp2p"},
  { "vob",   "video/mp2p"},
  { "mp2t",  "video/mp2t"},
  { "m1v",   "video/mpeg"},
  { "m2v",   "video/mpeg2"},
  { "mpg2",  "video/mpeg2"},
  { "mpeg2", "video/mpeg2"},
  { "m4v",   "video/mp4"},
  { "m4p",   "video/mp4"},
  { "mp4",   "video/mp4"},
  { "mp4ps", "video/x-nerodigital-ps"},
  { "ts",    "video/mpeg2"},
  { "ogm",   "video/mpeg"},
  { "mkv",   "video/mpeg"},
  { "rmvb",  "video/mpeg"},
  { "mov",   "video/quicktime"},
  { "hdmov", "video/quicktime"},
  { "qt",    "video/quicktime"},
  { "bin",   "video/mpeg2"},
  { "iso",   "video/mpeg2"},

  /* Audio files */
  { "3gp",  "audio/3gpp"},
  { "aac",  "audio/x-aac"},
  { "ac3",  "audio/x-ac3"},
  { "aif",  "audio/aiff"},
  { "aiff", "audio/aiff"},
  { "at3p", "audio/x-atrac3"},
  { "au",   "audio/basic"},
  { "snd",  "audio/basic"},
  { "dts",  "audio/x-dts"},
  { "rmi",  "audio/midi"},
  { "mid",  "audio/midi"},
  { "mp1",  "audio/mp1"},
  { "mp2",  "audio/mp2"},
  { "mp3",  "audio/mpeg"},
  { "m4a",  "audio/mp4"},
  { "ogg",  "audio/x-ogg"},
  { "wav",  "audio/wav"},
  { "pcm",  "audio/l16"},
  { "lpcm", "audio/l16"},
  { "l16",  "audio/l16"},
  { "wma",  "audio/x-ms-wma"},
  { "mka",  "audio/mpeg"},
  { "ra",   "audio/x-pn-realaudio"},
  { "rm",   "audio/x-pn-realaudio"},
  { "ram",  "audio/x-pn-realaudio"},
  { "flac", "audio/x-flac"},

  /* Images files */
  { "bmp",  "image/bmp"},
  { "ico",  "image/x-icon"},
  { "gif",  "image/gif"},
  { "jpeg", "image/jpeg"},
  { "jpg",  "image/jpeg"},
  { "jpe",  "image/jpeg"},
  { "pcd",  "image/x-ms-bmp"},
  { "png",  "image/png"},
  { "pnm",  "image/x-portable-anymap"},
  { "ppm",  "image/x-portable-pixmap"},
  { "qti",  "image/x-quicktime"},
  { "qtf",  "image/x-quicktime"},
  { "qtif", "image/x-quicktime"},
  { "tif",  "image/tiff"},
  { "tiff", "image/tiff"},

  { NULL,   NULL}
};

extern dlna_registered_profile_t dlna_profile_image_jpeg;
extern dlna_registered_profile_t dlna_profile_image_png;
extern dlna_registered_profile_t dlna_profile_audio_ac3;
extern dlna_registered_profile_t dlna_profile_audio_amr;
extern dlna_registered_profile_t dlna_profile_audio_atrac3;
extern dlna_registered_profile_t dlna_profile_audio_lpcm;
extern dlna_registered_profile_t dlna_profile_audio_mp3;
extern dlna_registered_profile_t dlna_profile_audio_mpeg4;
extern dlna_registered_profile_t dlna_profile_audio_wma;
extern dlna_registered_profile_t dlna_profile_av_mpeg1;
extern dlna_registered_profile_t dlna_profile_av_mpeg2;
extern dlna_registered_profile_t dlna_profile_av_mpeg4_part2;
extern dlna_registered_profile_t dlna_profile_av_mpeg4_part10;
extern dlna_registered_profile_t dlna_profile_av_wmv9;

static void
dlna_register_profile (dlna_t *dlna, dlna_registered_profile_t *profile)
{
  void **p;

  if (!dlna)
    return;

  if (!dlna->inited)
    dlna = dlna_init ();
  
  p = &dlna->first_profile;
  while (*p != NULL)
  {
    if (((dlna_registered_profile_t *) *p)->id == profile->id)
      return; /* already registered */
    p = (void *) &((dlna_registered_profile_t *) *p)->next;
  }
  *p = profile;
  profile->next = NULL;
}

void
dlna_register_all_media_profiles (dlna_t *dlna)
{
  if (!dlna)
    return;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  dlna_register_profile (dlna, &dlna_profile_image_jpeg);
  dlna_register_profile (dlna, &dlna_profile_image_png);
  dlna_register_profile (dlna, &dlna_profile_audio_ac3);
  dlna_register_profile (dlna, &dlna_profile_audio_amr);
  dlna_register_profile (dlna, &dlna_profile_audio_atrac3);
  dlna_register_profile (dlna, &dlna_profile_audio_lpcm);
  dlna_register_profile (dlna, &dlna_profile_audio_mp3);
  dlna_register_profile (dlna, &dlna_profile_audio_mpeg4);
  dlna_register_profile (dlna, &dlna_profile_audio_wma);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg1);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg2);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part2);
  dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part10);
  dlna_register_profile (dlna, &dlna_profile_av_wmv9);
}

void
dlna_register_media_profile (dlna_t *dlna, dlna_media_profile_t profile)
{
  if (!dlna)
    return;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  switch (profile)
  {
  case DLNA_PROFILE_IMAGE_JPEG:
    dlna_register_profile (dlna, &dlna_profile_image_jpeg);
    break;
  case DLNA_PROFILE_IMAGE_PNG:
    dlna_register_profile (dlna, &dlna_profile_image_png);
    break;
  case DLNA_PROFILE_AUDIO_AC3:
    dlna_register_profile (dlna, &dlna_profile_audio_ac3);
    break;
  case DLNA_PROFILE_AUDIO_AMR:
    dlna_register_profile (dlna, &dlna_profile_audio_amr);
    break;
  case DLNA_PROFILE_AUDIO_ATRAC3:
    dlna_register_profile (dlna, &dlna_profile_audio_atrac3);
    break;
  case DLNA_PROFILE_AUDIO_LPCM:
    dlna_register_profile (dlna, &dlna_profile_audio_lpcm);
    break;
  case DLNA_PROFILE_AUDIO_MP3:
    dlna_register_profile (dlna, &dlna_profile_audio_mp3);
    break;
  case DLNA_PROFILE_AUDIO_MPEG4:
    dlna_register_profile (dlna, &dlna_profile_audio_mpeg4);
    break;
  case DLNA_PROFILE_AUDIO_WMA:
    dlna_register_profile (dlna, &dlna_profile_audio_wma);
    break;
  case DLNA_PROFILE_AV_MPEG1:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg1);
    break;
  case DLNA_PROFILE_AV_MPEG2:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg2);
    break;
  case DLNA_PROFILE_AV_MPEG4_PART2:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part2);
    break;
  case DLNA_PROFILE_AV_MPEG4_PART10:
    dlna_register_profile (dlna, &dlna_profile_av_mpeg4_part10);
    break;
  case DLNA_PROFILE_AV_WMV9:
    dlna_register_profile (dlna, &dlna_profile_av_wmv9);
    break;
  default:
    break;
  }
}

static av_codecs_t *
av_profile_get_codecs (AVFormatContext *ctx)
{
  av_codecs_t *codecs = NULL;
  unsigned int i;
  int audio_stream = -1, video_stream = -1;
 
  codecs = malloc (sizeof (av_codecs_t));

  for (i = 0; i < ctx->nb_streams; i++)
  {
    if (audio_stream == -1 &&
        ctx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
    {
      audio_stream = i;
      continue;
    }
    else if (video_stream == -1 &&
             ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
    {
      video_stream = i;
      continue;
    }
  }

  codecs->as = audio_stream >= 0 ? ctx->streams[audio_stream] : NULL;
  codecs->ac = audio_stream >= 0 ? ctx->streams[audio_stream]->codec : NULL;

  codecs->vs = video_stream >= 0 ? ctx->streams[video_stream] : NULL;
  codecs->vc = video_stream >= 0 ? ctx->streams[video_stream]->codec : NULL;

  /* check for at least one video stream and one audio stream in container */
  if (!codecs->ac && !codecs->vc)
  {
    free (codecs);
    return NULL;
  }
  
  return codecs;
}

static int
match_file_extension (const char *filename, const char *extensions)
{
  const char *ext, *p;
  char ext1[32], *q;

  if (!filename)
    return 0;

  ext = strrchr (filename, '.');
  if (ext)
  {
    ext++;
    p = extensions;
    for (;;)
    {
      q = ext1;
      while (*p != '\0' && *p != ',' && (q - ext1 < (int) sizeof (ext1) - 1))
        *q++ = *p++;
      *q = '\0';
      if (!strcasecmp (ext1, ext))
        return 1;
      if (*p == '\0')
        break;
      p++;
    }
  }
  
  return 0;
}

dlna_profile_t *
dlna_guess_media_profile (dlna_t *dlna, const char *filename)
{
  AVFormatContext *ctx;
  dlna_registered_profile_t *p;
  dlna_profile_t *profile = NULL;
  dlna_container_type_t st;
  av_codecs_t *codecs;

  if (!dlna)
    return NULL;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  if (av_open_input_file (&ctx, filename, NULL, 0, NULL) != 0)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "can't open file: %s\n", filename);
    return NULL;
  }

  if (av_find_stream_info (ctx) < 0)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "can't find stream info\n");
    av_close_input_file (ctx);
    return NULL;
  }

#ifdef HAVE_DEBUG
  dump_format (ctx, 0, NULL, 0);
#endif /* HAVE_DEBUG */

  /* grab codecs info */
  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
  {
    av_close_input_file (ctx);
    return NULL;
  }

  /* check for container type */
  st = stream_get_container (dlna, ctx);
  
  p = dlna->first_profile;
  while (p)
  {
    dlna_profile_t *prof;
    
    if (dlna->check_extensions)
    {
      if (p->extensions)
      {
        /* check for valid file extension */
        if (!match_file_extension (filename, p->extensions))
        {
          p = p->next;
          continue;
        }
      }
    }
    
    prof = p->probe (ctx, st, codecs);
    if (prof)
    {
      profile = prof;
      profile->media_class = p->class;
      break;
    }
    p = p->next;
  }

  av_close_input_file (ctx);
  free (codecs);
  return profile;
}

static dlna_profile_t *
upnp_guess_media_profile (dlna_t *dlna,
                          const char *filename, AVFormatContext *ctx)
{
  dlna_profile_t *profile = NULL;
  av_codecs_t *codecs;
  char *extension;
  int i;
  
  if (!dlna || !ctx)
    return NULL;

  extension = get_file_extension (filename);
  if (!extension)
    return NULL;
  
  /* grab codecs info */
  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
    return NULL;
  
  profile = malloc (sizeof (dlna_profile_t));
  profile->id = NULL; /* obviously not DLNA compliant */
  profile->label = NULL;
  
  profile->mime = "";
  for (i = 0; mime_type_list[i].extension; i++)
    if (!strcmp (extension, mime_type_list[i].extension))
      profile->mime = mime_type_list[i].mime;

  profile->media_class = DLNA_CLASS_UNKNOWN;
  if (stream_ctx_is_av (codecs))
    profile->media_class = DLNA_CLASS_AV;
  else if (stream_ctx_is_audio (codecs))
    profile->media_class = DLNA_CLASS_AUDIO;
  else if (ctx->nb_streams > 1 && codecs->vc && !codecs->ac)
    profile->media_class = DLNA_CLASS_IMAGE;
  
  free (codecs);
  
  return profile;
}

static dlna_properties_t *
dlna_item_get_properties (AVFormatContext *ctx)
{
  dlna_properties_t *prop;
  av_codecs_t *codecs;
  int duration, hours, min, sec;
  
  if (!ctx)
    return NULL;

  /* grab codecs info */
  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
    return NULL;
  
  prop = malloc (sizeof (dlna_properties_t));
  prop->size = ctx->file_size;

  duration = (int) (ctx->duration / AV_TIME_BASE);
  hours = (int) (duration / 3600);
  min = (int) ((duration - (hours * 3600)) / 60);
  sec = (int) (duration - (hours * 3600) - (min * 60));
  memset (prop->duration, '\0', 64);
  if (hours)
    sprintf (prop->duration, "%d:%.2d:%.2d.", hours, min, sec);
  else
    sprintf (prop->duration, ":%.2d:%.2d.", min, sec);

  prop->bitrate = (uint32_t) (ctx->bit_rate / 8);
  prop->sample_frequency = codecs->ac ? codecs->ac->sample_rate : 0;
  prop->bps = codecs->ac ? codecs->ac->bits_per_sample : 0;
  prop->channels = codecs->ac ? codecs->ac->channels : 0;

  memset (prop->resolution, '\0', 64);
  if (codecs->vc)
    sprintf (prop->resolution, "%dx%d",
             codecs->vc->width, codecs->vc->height);

  free (codecs);
  return prop;
}

static dlna_metadata_t *
dlna_item_get_metadata (AVFormatContext *ctx)
{
  dlna_metadata_t *meta;
  
  if (!ctx)
    return NULL;

  meta = malloc (sizeof (dlna_metadata_t));
  meta->title   = strdup (ctx->title);
  meta->author  = strdup (ctx->author);
  meta->comment = strdup (ctx->comment);
  meta->album   = strdup (ctx->album);
  meta->track   = ctx->track;
  meta->genre   = strdup (ctx->genre);

  return meta;
}

static void
dlna_metadata_free (dlna_metadata_t *meta)
{
  if (!meta)
    return;

  if (meta->title)
    free (meta->title);
  if (meta->author)
    free (meta->author);
  if (meta->comment)
    free (meta->comment);
  if (meta->album)
    free (meta->album);
  if (meta->genre)
    free (meta->genre);
  free (meta);
}

dlna_item_t *
dlna_item_new (dlna_t *dlna, const char *filename)
{
  AVFormatContext *ctx;
  dlna_item_t *item;

  if (!dlna || !filename)
    return NULL;
  
  if (!dlna->inited)
    dlna = dlna_init ();
  
  if (av_open_input_file (&ctx, filename, NULL, 0, NULL) != 0)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "can't open file: %s\n", filename);
    return NULL;
  }

  if (av_find_stream_info (ctx) < 0)
  {
    dlna_log (dlna, DLNA_MSG_CRITICAL, "can't find stream info\n");
    av_close_input_file (ctx);
    return NULL;
  }

  item = malloc (sizeof (dlna_item_t));
  if (dlna->mode == DLNA_CAPABILITY_DLNA)
    item->profile    = dlna_guess_media_profile (dlna, filename);
  else
    item->profile    = upnp_guess_media_profile (dlna, filename, ctx);
  if (!item->profile) /* not DLNA compliant */
  {
    free (item);
    av_close_input_file (ctx);
    return NULL;
  }
  item->filename   = strdup (filename);
  item->properties = dlna_item_get_properties (ctx);
  item->metadata   = dlna_item_get_metadata (ctx);
  item->media_class= item->profile->media_class;

  av_close_input_file (ctx);

  return item;
}

void
dlna_item_free (dlna_item_t *item)
{
  if (!item)
    return;

  if (item->filename)
    free (item->filename);
  if (item->properties)
    free (item->properties);
  dlna_metadata_free (item->metadata);
  item->profile = NULL;
  free (item);
}

/* UPnP ContentDirectory Object Item */
#define UPNP_OBJECT_ITEM_PHOTO            "object.item.imageItem.photo"
#define UPNP_OBJECT_ITEM_AUDIO            "object.item.audioItem.musicTrack"
#define UPNP_OBJECT_ITEM_VIDEO            "object.item.videoItem.movie"

char *
dlna_profile_upnp_object_item (dlna_profile_t *profile)
{
  if (!profile)
    return NULL;

  switch (profile->media_class)
  {
  case DLNA_CLASS_IMAGE:
    return UPNP_OBJECT_ITEM_PHOTO;
  case DLNA_CLASS_AUDIO:
    return UPNP_OBJECT_ITEM_AUDIO;
  case DLNA_CLASS_AV:
    return UPNP_OBJECT_ITEM_VIDEO;
  default:
    break;
  }

  return NULL;
}

int
stream_ctx_is_image (AVFormatContext *ctx,
                     av_codecs_t *codecs, dlna_container_type_t st)
{
  /* should only have 1 stream */
  if (ctx->nb_streams > 1)
    return 0;

  /* should be inside image container */
  if (st != CT_IMAGE)
    return 0;

  if (!codecs->vc)
    return 0;

  return 1;
}

int
stream_ctx_is_audio (av_codecs_t *codecs)
{
  /* we need an audio codec ... */
  if (!codecs->ac)
    return 0;

  /* ... but no video one */
  if (codecs->vc)
    return 0;

  return 1;
}

int
stream_ctx_is_av (av_codecs_t *codecs)
{
  if (!codecs->as || !codecs->ac || !codecs->vs || !codecs->vc)
    return 0;

  return 1;
}

char *
get_file_extension (const char *filename)
{
  char *str = NULL;

  str = strrchr (filename, '.');
  if (str)
    str++;

  return str;
}

audio_profile_t
audio_profile_guess (AVCodecContext *ac)
{
  audio_profile_t ap = AUDIO_PROFILE_INVALID;
  
  if (!ac)
    return ap;

  ap = audio_profile_guess_aac (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_ac3 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_amr (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_atrac (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_g726 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_lpcm (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_mp2 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_mp3 (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  ap = audio_profile_guess_wma (ac);
  if (ap != AUDIO_PROFILE_INVALID)
    return ap;

  return AUDIO_PROFILE_INVALID;
}
