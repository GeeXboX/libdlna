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

/* Profile for audio media class content */
static dlna_profile_t aac_adts = {
  .id = "AAC_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_adts_320 = {
  .id = "AAC_ADTS_320",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso = {
  .id = "AAC_ISO",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t aac_iso_320 = {
  .id = "AAC_ISO_320",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content. In the case of AAC LTP profiles,
   both the ISO file formats and the ADTS format are supported by
   the same profile. */
static dlna_profile_t aac_ltp_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_ltp_mult5_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 7.1 channels */
static dlna_profile_t aac_ltp_mult7_iso __attribute__ ((unused)) = {
  .id = "AAC_LTP_MULT7_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_adts = {
  .id = "AAC_MULT5_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t aac_mult5_iso = {
  .id = "AAC_MULT5_ISO",
  .mime = MIME_AUDIO_MPEG_4,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_adts __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l3_iso __attribute__ ((unused)) = {
  .id = "HEAAC_L3_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_adts __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ADTS",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t heaac_mult5_iso __attribute__ ((unused)) = {
  .id = "HEAAC_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_adts_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ADTS_320",
  .mime = MIME_AUDIO_ADTS,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t heaac_l2_iso_320 __attribute__ ((unused)) = {
  .id = "HEAAC_L2_ISO_320",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content */
static dlna_profile_t bsac_iso __attribute__ ((unused)) = {
  .id = "BSAC_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_2CH
};

/* Profile for audio media class content with up to 5.1 channels */
static dlna_profile_t bsac_mult5_iso __attribute__ ((unused)) = {
  .id = "BSAC_MULT5_ISO",
  .mime = NULL,
  .label = LABEL_AUDIO_MULTI
};

audio_profile_t
audio_profile_guess_aac (AVCodecContext *ac)
{
  if (!ac)
    return AUDIO_PROFILE_INVALID;

  if (!ac)
    return AUDIO_PROFILE_INVALID;
  
  /* TODO: need to check for HE-AAC, LTP and BSAC */
  if (ac->codec_id != CODEC_ID_AAC)
    return AUDIO_PROFILE_INVALID;
  
  /* supported sampling rate:
     8, 11.025, 12, 16, 22.05, 24, 32, 44.1 and 48 kHz */
  if (ac->sample_rate != 8000 &&
      ac->sample_rate != 11025 &&
      ac->sample_rate != 12000 &&
      ac->sample_rate != 16000 &&
      ac->sample_rate != 22050 &&
      ac->sample_rate != 24000 &&
      ac->sample_rate != 32000 &&
      ac->sample_rate != 44100 &&
      ac->sample_rate != 48000)
    return AUDIO_PROFILE_INVALID;

  switch (ac->channels)
  {
  case 1:
  case 2:
    if (ac->bit_rate <= 576000)
      return AUDIO_PROFILE_AAC;
    break;
  case 5:
    if (ac->bit_rate <= 1444000)
      return AUDIO_PROFILE_AAC_MULT5;
    break;
  case 7:
    return AUDIO_PROFILE_AAC_LTP_MULT7;
  default:
    break;
  }
  
  return AUDIO_PROFILE_INVALID;
}

static dlna_profile_t *
probe_mpeg4 (AVFormatContext *ctx)
{
  AVCodecContext *codec = NULL;
  audio_profile_t ap;
  int adts = 0;
  
  if (!strcasecmp (get_file_extension (ctx->filename), "aac"))
    adts = 1;

  codec = audio_profile_get_codec (ctx);
  if (!codec)
    return NULL;
  
  /* check for AAC codec */
  /* TODO: need to check for HE-AAC, LTP and BSAC */
  ap = audio_profile_guess_aac (codec);
  if (ap == AUDIO_PROFILE_INVALID)
    return NULL;
  
  if (codec->codec_id == CODEC_ID_AAC)
  {
    if (ap == AUDIO_PROFILE_AAC_MULT5)
      return adts ?
        set_profile (&aac_mult5_adts): set_profile (&aac_mult5_iso);
    else if (AUDIO_PROFILE_AAC)
      return adts ?
        set_profile (&aac_adts_320) : set_profile (&aac_iso_320);
    else
      return adts ?
        set_profile (&aac_adts) : set_profile (&aac_iso);
  }

  return NULL;
}

dlna_registered_profile_t dlna_profile_audio_mpeg4 = {
  .id = DLNA_PROFILE_AUDIO_MPEG4,
  .extensions = "aac,3gp,mp4,mov,qt,m4a",
  .probe = probe_mpeg4,
  .next = NULL
};
