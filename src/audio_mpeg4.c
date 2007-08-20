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

#define MPEG4_KNOWN_EXTENSIONS "aac,3gp,mp4,mov,qt,m4a"

#define MPEG4_MIME_TYPE "audio/mp4"
#define ADTS_MIME_TYPE "audio/vnd.dlna.adts"
#define THREE_GPP_MIME_TYPE "audio/3gpp"

#define MPEG4_2CH_LABEL "2-ch"
#define MPEG4_MULTI_CH_LABEL "multi"

/* Profile for audio media class content */
static dlna_profile_t aac_adts = {
  .id = "AAC_ADTS",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t aac_adts_320 = {
  .id = "AAC_ADTS_320",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso = {
  .id = "AAC_ISO",
  .mime = MPEG4_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso_320 = {
  .id = "AAC_ISO_320",
  .mime = MPEG4_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content. In the case of AAC LTP profiles,
   both the ISO file formats and the ADTS format are supported by
   the same profile. */
static dlna_profile_t aac_ltp_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_ISO",
  .mime = NULL,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_ltp_mult5_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT5_ISO",
  .mime = NULL,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content with up to 7.1 channels */
static dlna_profile_t aac_ltp_mult7_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT7_ISO",
  .mime = NULL,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_adts = {
  .id = "AAC_MULT5_ADTS",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_iso = {
  .id = "AAC_MULT5_ISO",
  .mime = MPEG4_MIME_TYPE,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO",
  .mime = NULL,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ADTS",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ISO",
  .mime = NULL,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_adts __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ADTS",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_iso __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ISO",
  .mime = NULL,
  .label = MPEG4_MULTI_CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS_320",
  .mime = ADTS_MIME_TYPE,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO_320",
  .mime = NULL,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content */
static dlna_profile_t bsac_iso __attribute__ ((unused)) = {
  .id = "BSAC_ISO",
  .mime = NULL,
  .label = MPEG4_2CH_LABEL
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t bsac_mult5_iso __attribute__ ((unused)) = {
  .id = "BSAC_MULT5_ISO",
  .mime = NULL,
  .label = MPEG4_MULTI_CH_LABEL
};

static dlna_profile_t *
probe_mpeg4 (AVFormatContext *ctx)
{
  AVStream *stream = NULL;
  AVCodecContext *codec = NULL;
  int adts = 0;
  int i;
  
  /* check for valid file extension */
  if (!match_file_extension (ctx->filename, MPEG4_KNOWN_EXTENSIONS))
    return NULL;

  if (!strcasecmp (get_file_extension (ctx->filename), "aac"))
    adts = 1;

  /* check there is no video stream in container */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    AVStream *s;
    AVCodecContext *c;

    s = ctx->streams[i];
    c = s->codec;

    if (c->codec_type == CODEC_TYPE_VIDEO)
      return NULL;
  }

  /* find first audio stream */
  for (i = 0; i < ctx->nb_streams; i++)
  {
    stream = ctx->streams[i];
    if (stream)
      codec = stream->codec;

    if (codec && codec->codec_type == CODEC_TYPE_AUDIO)
      break;
  }

  if (!codec)
    return NULL;
  
  /* check for AAC codec */
  /* TODO: need to check for HE-AAC, LTP and BSAC */
  if (codec->codec_id == CODEC_ID_AAC)
  {
    /* supported sampling rate:
       8, 11.025, 12, 16, 22.05, 24, 32, 44.1 and 48 kHz */
    if (codec->sample_rate != 8000 &&
        codec->sample_rate != 11025 &&
        codec->sample_rate != 12000 &&
        codec->sample_rate != 16000 &&
        codec->sample_rate != 22050 &&
        codec->sample_rate != 24000 &&
        codec->sample_rate != 32000 &&
        codec->sample_rate != 44100 &&
        codec->sample_rate != 48000)
    return NULL;

    if (codec->channels == 5)
    {
      /* maximum bitrate: 1440 Kbps */
      if (codec->bit_rate > 1440000)
        return NULL;
      
      return adts ?
        set_profile (&aac_mult5_adts): set_profile (&aac_mult5_iso);
    }
    else if (codec->channels <= 2)
    {
      /* maximum bitrate: 576 Kbps */
      if (codec->bit_rate > 576000)
        return NULL;
      
      if (codec->bit_rate <= 320000)
        return adts ?
          set_profile (&aac_adts_320) : set_profile (&aac_iso_320);
    }
    else
      return adts ?
        set_profile (&aac_adts) : set_profile (&aac_iso);
  }

  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_mpeg4 = {
  .id = DLNA_PROFILE_AUDIO_MPEG4,
  .probe = probe_mpeg4,
  .next = NULL
};
