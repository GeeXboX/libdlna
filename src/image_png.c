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
#include <string.h>

#include "dlna.h"
#include "profiles.h"

/* Profile for image thumbnails */
static dlna_profile_t png_tn = {
  .id = "PNG_TN",
  .mime = MIME_IMAGE_PNG,
  .label = LABEL_IMAGE_ICON
};

/* Profile for small icons */
static dlna_profile_t png_sm_ico = {
  .id = "PNG_SM_ICO",
  .mime = MIME_IMAGE_PNG,
  .label = LABEL_IMAGE_ICON
};

/* Profile for large icons */
static dlna_profile_t png_lrg_ico = {
  .id = "PNG_LRG_ICO",
  .mime = MIME_IMAGE_PNG,
  .label = LABEL_IMAGE_ICON
};

/* Profile for image class content of high resolution */
static dlna_profile_t png_lrg = {
  .id = "PNG_LRG",
  .mime = MIME_IMAGE_PNG,
  .label = LABEL_IMAGE_PICTURE
};

static dlna_profile_t *
probe_png (AVFormatContext *ctx)
{
  AVStream *stream;
  AVCodecContext *codec;

  /* should only have 1 stream */
  if (ctx->nb_streams > 1)
    return NULL;

  stream = ctx->streams[0];
  codec = stream->codec;

  /* which should be a video one (even for images) */
  if (codec->codec_type != CODEC_TYPE_VIDEO)
    return NULL;

  /* check for PNG compliant codec */
  if (codec->codec_id != CODEC_ID_PNG)
    return NULL;
  
  if (codec->width <= 48 && codec->height <= 48)
    return set_profile (&png_sm_ico);
  else if (codec->width <= 120 && codec->height <= 120)
    return set_profile (&png_lrg_ico);
  else if (codec->width <= 160 && codec->height <= 160)
    return set_profile (&png_tn);
  else if (codec->width <= 4096 && codec->height <= 4096)
    return set_profile (&png_lrg);
  
  return NULL;
}

dlna_registered_profile_t dlna_profile_image_png = {
  .id = DLNA_PROFILE_IMAGE_PNG,
  .class = DLNA_CLASS_IMAGE,
  .extensions = "png",
  .probe = probe_png,
  .next = NULL
};
