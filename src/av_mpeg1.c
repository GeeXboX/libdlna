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

/* MPEG-1 video with 2 channel MPEG-1 Layer2 audio
   encapsulated in MPEG-1 system */
static dlna_profile_t mpeg1 = {
  .id = "MPEG1",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t *
probe_mpeg1 (AVFormatContext *ctx)
{
  av_codecs_t *codecs;
  
  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
    goto probe_mpeg1_end;

  /* check for MPEG-1 video codec */
  if (codecs->vc->codec_id != CODEC_ID_MPEG1VIDEO)
    goto probe_mpeg1_end;

  /* video bitrate must be CBR at 1,151,929.1 bps */
  if (codecs->vc->bit_rate != 1150000)
    goto probe_mpeg1_end;

  /* supported resolutions:
     - 352x288 @ 25 Hz (PAL)
     - 352x240 @ 29.97 Hz (NTSC)
     - 352x240 @ 23.976 Hz
  */
  if (codecs->vc->width == 352 && codecs->vc->height == 288)
  {
    if (codecs->vs->r_frame_rate.num != 25 &&
        codecs->vs->r_frame_rate.den != 1)
          goto probe_mpeg1_end;
  }
  else if (codecs->vc->width == 352 && codecs->vc->height == 240)
  {
    if ((codecs->vs->r_frame_rate.num != 30000 &&
         codecs->vs->r_frame_rate.den != 1001) ||
        (codecs->vs->r_frame_rate.num != 24000 &&
         codecs->vs->r_frame_rate.den != 1001))
          goto probe_mpeg1_end;
  }
  else
    goto probe_mpeg1_end;

  /* check for MPEG-1 Layer-2 audio codec */
  if (codecs->ac->codec_id != CODEC_ID_MP2)
    goto probe_mpeg1_end;
  
  /* supported channels: stereo only */
  if (codecs->ac->channels != 2)
    goto probe_mpeg1_end;

  /* supported sampling rate: 44.1 kHz only */
  if (codecs->ac->sample_rate != 44100)
    goto probe_mpeg1_end;

  /* supported bitrate: 224 Kbps only */
  if (codecs->ac->bit_rate != 224000)
    goto probe_mpeg1_end;

  return set_profile (&mpeg1);

 probe_mpeg1_end:
  if (codecs)
    free (codecs);
  return NULL;
}

dlna_registered_profile_t dlna_profile_av_mpeg1 = {
  .id = DLNA_PROFILE_AV_MPEG1,
  .extensions = "mpg,mpeg,mpe,m1v",
  .probe = probe_mpeg1,
  .next = NULL
};
