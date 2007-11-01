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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ffmpeg/avcodec.h>

#include "dlna.h"
#include "profiles.h"
#include "containers.h"

typedef enum {
  AVC_VIDEO_PROFILE_INVALID,
  AVC_VIDEO_PROFILE_BL_QCIF15,
  AVC_VIDEO_PROFILE_BL_L1B_QCIF,
  AVC_VIDEO_PROFILE_BL_L12_CIF15,
  AVC_VIDEO_PROFILE_BL_CIF15,
  AVC_VIDEO_PROFILE_BL_L2_CIF30,
  AVC_VIDEO_PROFILE_BL_CIF30,
  AVC_VIDEO_PROFILE_BL_L3L_SD,
  AVC_VIDEO_PROFILE_BL_L3_SD,
  AVC_VIDEO_PROFILE_MP_SD,
  AVC_VIDEO_PROFILE_MP_HD
} avc_video_profile_t;

typedef enum {
  AVC_AUDIO_PROFILE_INVALID,
  AVC_AUDIO_PROFILE_AMR_WB,
  AVC_AUDIO_PROFILE_AMR,
  AVC_AUDIO_PROFILE_ATRAC,
  AVC_AUDIO_PROFILE_BSAC_MULT5,
  AVC_AUDIO_PROFILE_BSAC,
  AVC_AUDIO_PROFILE_AAC_LTP_MULT7,
  AVC_AUDIO_PROFILE_AAC_LTP_MULT5,
  AVC_AUDIO_PROFILE_AAC_LTP_520,
  AVC_AUDIO_PROFILE_AAC_LTP,
  AVC_AUDIO_PROFILE_AC3,
  AVC_AUDIO_PROFILE_MP3,
  AVC_AUDIO_PROFILE_HEAAC,  
  AVC_AUDIO_PROFILE_AAC_MULT5,
  AVC_AUDIO_PROFILE_AAC_940,
  AVC_AUDIO_PROFILE_AAC_540,
  AVC_AUDIO_PROFILE_AAC_520,
  AVC_AUDIO_PROFILE_AAC
} avc_audio_profile_t;

/********************/
/* MPEG-4 Container */
/********************/

static dlna_profile_t avc_mp4_mp_sd_aac_mult5 = {
  .id = "AVC_MP4_MP_SD_AAC_MULT5",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_heaac_l2 = {
  .id = "AVC_MP4_MP_SD_HEAAC_L2",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_mpeg1_l3 = {
  .id = "AVC_MP4_MP_SD_MPEG1_L3",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_ac3 = {
  .id = "AVC_MP4_MP_SD_AC3",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_aac_ltp = {
  .id = "AVC_MP4_MP_SD_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_aac_ltp_mult5 = {
  .id = "AVC_MP4_MP_SD_AAC_LTP_MULT5",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_aac_ltp_mult7 = {
  .id = "AVC_MP4_MP_SD_AAC_LTP_MULT7",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_atrac3plus = {
  .id = "AVC_MP4_MP_SD_ATRAC3plus",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_bl_l3l_sd_aac = {
  .id = "AVC_MP4_BL_L3L_SD_AAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_bl_l3l_sd_heaac = {
  .id = "AVC_MP4_BL_L3L_SD_HEAAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_bl_l3_sd_aac = {
  .id = "AVC_MP4_BL_L3_SD_AAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_mp_sd_bsac = {
  .id = "AVC_MP4_MP_SD_BSAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_mp4_bl_cif30_aac_mult5 = {
  .id = "AVC_MP4_BL_CIF30_AAC_MULT5",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_heaac_l2 = {
  .id = "AVC_MP4_BL_CIF30_HEAAC_L2",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_mpeg1_l3 = {
  .id = "AVC_MP4_BL_CIF30_MPEG1_L3",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_ac3 = {
  .id = "AVC_MP4_BL_CIF30_AC3",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_aac_ltp = {
  .id = "AVC_MP4_BL_CIF30_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_aac_ltp_mult5 = {
  .id = "AVC_MP4_BL_CIF30_AAC_LTP_MULT5",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_l2_cif30_aac = {
  .id = "AVC_MP4_BL_L2_CIF30_AAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_bsac = {
  .id = "AVC_MP4_BL_CIF30_BSAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif30_bsac_mult5 = {
  .id = "AVC_MP4_BL_CIF30_BSAC_MULT5",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_mp4_bl_cif15_heaac = {
  .id = "AVC_MP4_BL_CIF15_HEAAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_amr = {
  .id = "AVC_MP4_BL_CIF15_AMR",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_aac = {
  .id = "AVC_MP4_BL_CIF15_AAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_aac_520 = {
  .id = "AVC_MP4_BL_CIF15_AAC_520",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_aac_ltp = {
  .id = "AVC_MP4_BL_CIF15_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_aac_ltp_520 = {
  .id = "AVC_MP4_BL_CIF15_AAC_LTP_520",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_cif15_bsac = {
  .id = "AVC_MP4_BL_CIF15_BSAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_l12_cif15_heaac = {
  .id = "AVC_MP4_BL_L12_CIF15_HEAAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_mp4_bl_l1b_qcif15_heaac = {
  .id = "AVC_MP4_BL_L1B_QCIF15_HEAAC",
  .mime = MIME_VIDEO_MPEG_4,
  .label = LABEL_VIDEO_QCIF15
};

/*********************/
/* MPEG-TS Container */
/*********************/

static dlna_profile_t avc_ts_mp_sd_aac_mult5 = {
  .id = "AVC_TS_MP_SD_AAC_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_mult5_t = {
  .id = "AVC_TS_MP_SD_AAC_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_mult5_iso = {
  .id = "AVC_TS_MP_SD_AAC_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_heaac_l2 = {
  .id = "AVC_TS_MP_SD_HEAAC_L2",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_heaac_l2_t = {
  .id = "AVC_TS_MP_SD_HEAAC_L2_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_heaac_l2_iso = {
  .id = "AVC_TS_MP_SD_HEAAC_L2_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_mpeg1_l3 = {
  .id = "AVC_TS_MP_SD_MPEG1_L3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_mpeg1_l3_t = {
  .id = "AVC_TS_MP_SD_MPEG1_L3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_mpeg1_l3_iso = {
  .id = "AVC_TS_MP_SD_MPEG1_L3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_ac3 = {
  .id = "AVC_TS_MP_SD_AC3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_ac3_t = {
  .id = "AVC_TS_MP_SD_AC3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_ac3_iso = {
  .id = "AVC_TS_MP_SD_AC3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp = {
  .id = "AVC_TS_MP_SD_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_t = {
  .id = "AVC_TS_MP_SD_AAC_LTP_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_iso = {
  .id = "AVC_TS_MP_SD_AAC_LTP_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult5 = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult5_t = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult5_iso = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult7 = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT7",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult7_t = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT7_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_aac_ltp_mult7_iso = {
  .id = "AVC_TS_MP_SD_AAC_LTP_MULT7_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_bsac = {
  .id = "AVC_TS_MP_SD_BSAC",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_bsac_t = {
  .id = "AVC_TS_MP_SD_BSAC_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_mp_sd_bsac_iso = {
  .id = "AVC_TS_MP_SD_BSAC_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_SD
};

static dlna_profile_t avc_ts_bl_cif30_aac_mult5 = {
  .id = "AVC_TS_BL_CIF30_AAC_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_mult5_t = {
  .id = "AVC_TS_BL_CIF30_AAC_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_mult5_iso = {
  .id = "AVC_TS_BL_CIF30_AAC_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_heaac_l2 = {
  .id = "AVC_TS_BL_CIF30_HEAAC_L2",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_heaac_l2_t = {
  .id = "AVC_TS_BL_CIF30_HEAAC_L2_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_heaac_l2_iso = {
  .id = "AVC_TS_BL_CIF30_HEAAC_L2_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_mpeg1_l3 = {
  .id = "AVC_TS_BL_CIF30_MPEG1_L3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_mpeg1_l3_t = {
  .id = "AVC_TS_BL_CIF30_MPEG1_L3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_mpeg1_l3_iso = {
  .id = "AVC_TS_BL_CIF30_MPEG1_L3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_ac3 = {
  .id = "AVC_TS_BL_CIF30_AC3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_ac3_t = {
  .id = "AVC_TS_BL_CIF30_AC3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_ac3_iso = {
  .id = "AVC_TS_BL_CIF30_AC3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp_t = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp_iso = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp_mult5 = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp_mult5_t = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_ltp_mult5_iso = {
  .id = "AVC_TS_BL_CIF30_AAC_LTP_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_940 = {
  .id = "AVC_TS_BL_CIF30_AAC_940",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_940_t = {
  .id = "AVC_TS_BL_CIF30_AAC_940_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_bl_cif30_aac_940_iso = {
  .id = "AVC_TS_BL_CIF30_AAC_940_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_ts_mp_hd_aac_mult5 = {
  .id = "AVC_TS_MP_HD_AAC_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_mult5_t = {
  .id = "AVC_TS_MP_HD_AAC_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_mult5_iso = {
  .id = "AVC_TS_MP_HD_AAC_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_heaac_l2 = {
  .id = "AVC_TS_MP_HD_HEAAC_L2",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_heaac_l2_t = {
  .id = "AVC_TS_MP_HD_HEAAC_L2_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_heaac_l2_iso = {
  .id = "AVC_TS_MP_HD_HEAAC_L2_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_mpeg1_l3 = {
  .id = "AVC_TS_MP_HD_MPEG1_L3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_mpeg1_l3_t = {
  .id = "AVC_TS_MP_HD_MPEG1_L3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_mpeg1_l3_iso = {
  .id = "AVC_TS_MP_HD_MPEG1_L3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_ac3 = {
  .id = "AVC_TS_MP_HD_AC3",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_ac3_t = {
  .id = "AVC_TS_MP_HD_AC3_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_ac3_iso = {
  .id = "AVC_TS_MP_HD_AC3_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac = {
  .id = "AVC_TS_MP_HD_AAC",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_t = {
  .id = "AVC_TS_MP_HD_AAC_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_iso = {
  .id = "AVC_TS_MP_HD_AAC_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp = {
  .id = "AVC_TS_MP_HD_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_t = {
  .id = "AVC_TS_MP_HD_AAC_LTP_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_iso = {
  .id = "AVC_TS_MP_HD_AAC_LTP_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult5 = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT5",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult5_t = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT5_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult5_iso = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT5_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult7 = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT7",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult7_t = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT7_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_mp_hd_aac_ltp_mult7_iso = {
  .id = "AVC_TS_MP_HD_AAC_LTP_MULT7_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_HD
};

static dlna_profile_t avc_ts_bl_cif15_aac = {
  .id = "AVC_TS_BL_CIF15_AAC",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_t = {
  .id = "AVC_TS_BL_CIF15_AAC_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_iso = {
  .id = "AVC_TS_BL_CIF15_AAC_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_540 = {
  .id = "AVC_TS_BL_CIF15_AAC_540",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_540_t = {
  .id = "AVC_TS_BL_CIF15_AAC_540_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_540_iso = {
  .id = "AVC_TS_BL_CIF15_AAC_540_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_ltp = {
  .id = "AVC_TS_BL_CIF15_AAC_LTP",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_ltp_t = {
  .id = "AVC_TS_BL_CIF15_AAC_LTP_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_aac_ltp_iso = {
  .id = "AVC_TS_BL_CIF15_AAC_LTP_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_bsac = {
  .id = "AVC_TS_BL_CIF15_BSAC",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_bsac_t = {
  .id = "AVC_TS_BL_CIF15_BSAC_T",
  .mime = MIME_VIDEO_MPEG_TS,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_ts_bl_cif15_bsac_iso = {
  .id = "AVC_TS_BL_CIF15_BSAC_ISO",
  .mime = MIME_VIDEO_MPEG,
  .label = LABEL_VIDEO_CIF15
};

/******************/
/* 3GPP Container */
/******************/

static dlna_profile_t avc_3gpp_bl_cif30_amr_wbplus = {
  .id = "AVC_3GPP_BL_CIF30_AMR_WBplus",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_CIF30
};

static dlna_profile_t avc_3gpp_bl_cif15_amr_wbplus = {
  .id = "AVC_3GPP_BL_CIF15_AMR_WBplus",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_CIF15
};

static dlna_profile_t avc_3gpp_bl_qcif15_aac = {
  .id = "AVC_3GPP_BL_QCIF15_AAC",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_QCIF15
};

static dlna_profile_t avc_3gpp_bl_qcif15_aac_ltp = {
  .id = "AVC_3GPP_BL_QCIF15_AAC_LTP",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_QCIF15
};

static dlna_profile_t avc_3gpp_bl_qcif15_heaac = {
  .id = "AVC_3GPP_BL_QCIF15_HEAAC",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_QCIF15
};

static dlna_profile_t avc_3gpp_bl_qcif15_amr_wbplus = {
  .id = "AVC_3GPP_BL_QCIF15_AMR_WBplus",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_QCIF15
};

static dlna_profile_t avc_3gpp_bl_qcif15_amr = {
  .id = "AVC_3GPP_BL_QCIF15_AMR",
  .mime = MIME_VIDEO_3GP,
  .label = LABEL_VIDEO_QCIF15
};

static const struct {
  dlna_profile_t *profile;
  dlna_container_type_t st;
  avc_video_profile_t vp;
  avc_audio_profile_t ap;
} avc_profiles_mapping[] = {
  /* MPEG-4 Container */
  { &avc_mp4_mp_sd_aac_mult5, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_mp4_mp_sd_heaac_l2, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_mp4_mp_sd_mpeg1_l3, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_MP3 },
  { &avc_mp4_mp_sd_ac3, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AC3 },
  { &avc_mp4_mp_sd_aac_ltp, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_mp4_mp_sd_aac_ltp_mult5, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_mp4_mp_sd_aac_ltp_mult7, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },
  { &avc_mp4_mp_sd_atrac3plus, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_ATRAC },
  { &avc_mp4_mp_sd_bsac, CT_MP4,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_BSAC },
  
  { &avc_mp4_bl_l3l_sd_aac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L3L_SD, AVC_AUDIO_PROFILE_AAC },
  { &avc_mp4_bl_l3l_sd_heaac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L3L_SD, AVC_AUDIO_PROFILE_HEAAC },

  { &avc_mp4_bl_l3_sd_aac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L3_SD, AVC_AUDIO_PROFILE_AAC },

  { &avc_mp4_bl_cif30_aac_mult5, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_mp4_bl_cif30_heaac_l2, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_mp4_bl_cif30_mpeg1_l3, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_MP3 },
  { &avc_mp4_bl_cif30_ac3, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AC3 },
  { &avc_mp4_bl_cif30_aac_ltp, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_mp4_bl_cif30_aac_ltp_mult5, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_mp4_bl_cif30_bsac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_BSAC },
  { &avc_mp4_bl_cif30_bsac_mult5, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_BSAC_MULT5 },
  
  { &avc_mp4_bl_l2_cif30_aac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L2_CIF30, AVC_AUDIO_PROFILE_AAC },
  
  { &avc_mp4_bl_cif15_heaac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_mp4_bl_cif15_amr, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AMR },
  { &avc_mp4_bl_cif15_aac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC },
  { &avc_mp4_bl_cif15_aac_520, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_520 },
  { &avc_mp4_bl_cif15_aac_ltp, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_mp4_bl_cif15_aac_ltp_520, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_LTP_520 },
  { &avc_mp4_bl_cif15_bsac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_BSAC },

  { &avc_mp4_bl_l12_cif15_heaac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L12_CIF15, AVC_AUDIO_PROFILE_HEAAC },

  { &avc_mp4_bl_l1b_qcif15_heaac, CT_MP4,
    AVC_VIDEO_PROFILE_BL_L1B_QCIF, AVC_AUDIO_PROFILE_HEAAC },

  /* MPEG-TS Container */
  { &avc_ts_mp_sd_aac_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_mp_sd_aac_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_mp_sd_aac_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_MULT5 },

  { &avc_ts_mp_sd_heaac_l2, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_mp_sd_heaac_l2_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_mp_sd_heaac_l2_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_HEAAC },

  { &avc_ts_mp_sd_mpeg1_l3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_mp_sd_mpeg1_l3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_mp_sd_mpeg1_l3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_MP3 },

  { &avc_ts_mp_sd_ac3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_mp_sd_ac3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_mp_sd_ac3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AC3 },

  { &avc_ts_mp_sd_aac_ltp, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_mp_sd_aac_ltp_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_mp_sd_aac_ltp_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP },

  { &avc_ts_mp_sd_aac_ltp_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_mp_sd_aac_ltp_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_mp_sd_aac_ltp_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },

  { &avc_ts_mp_sd_aac_ltp_mult7, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },
  { &avc_ts_mp_sd_aac_ltp_mult7_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },
  { &avc_ts_mp_sd_aac_ltp_mult7_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },

  { &avc_ts_mp_sd_bsac, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_BSAC },
  { &avc_ts_mp_sd_bsac_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_BSAC },
  { &avc_ts_mp_sd_bsac_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_SD, AVC_AUDIO_PROFILE_BSAC },

  { &avc_ts_bl_cif30_aac_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_bl_cif30_aac_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_bl_cif30_aac_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_MULT5 },

  { &avc_ts_bl_cif30_heaac_l2, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_bl_cif30_heaac_l2_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_bl_cif30_heaac_l2_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_HEAAC },

  { &avc_ts_bl_cif30_mpeg1_l3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_bl_cif30_mpeg1_l3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_bl_cif30_mpeg1_l3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_MP3 },

  { &avc_ts_bl_cif30_ac3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_bl_cif30_ac3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_bl_cif30_ac3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AC3 },

  { &avc_ts_bl_cif30_aac_ltp, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_bl_cif30_aac_ltp_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_bl_cif30_aac_ltp_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP },

  { &avc_ts_bl_cif30_aac_ltp_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_bl_cif30_aac_ltp_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_bl_cif30_aac_ltp_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },

  { &avc_ts_bl_cif30_aac_940, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_940 },
  { &avc_ts_bl_cif30_aac_940_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_940 },
  { &avc_ts_bl_cif30_aac_940_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AAC_940 },

  { &avc_ts_mp_hd_aac_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_mp_hd_aac_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_MULT5 },
  { &avc_ts_mp_hd_aac_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_MULT5 },

  { &avc_ts_mp_hd_heaac_l2, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_mp_hd_heaac_l2_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_ts_mp_hd_heaac_l2_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_HEAAC },

  { &avc_ts_mp_hd_mpeg1_l3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_mp_hd_mpeg1_l3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_MP3 },
  { &avc_ts_mp_hd_mpeg1_l3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_MP3 },

  { &avc_ts_mp_hd_ac3, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_mp_hd_ac3_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AC3 },
  { &avc_ts_mp_hd_ac3_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AC3 },

  { &avc_ts_mp_hd_aac, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC },
  { &avc_ts_mp_hd_aac_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC },
  { &avc_ts_mp_hd_aac_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC },

  { &avc_ts_mp_hd_aac_ltp, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_mp_hd_aac_ltp_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_mp_hd_aac_ltp_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP },

  { &avc_ts_mp_hd_aac_ltp_mult5, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_mp_hd_aac_ltp_mult5_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },
  { &avc_ts_mp_hd_aac_ltp_mult5_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT5 },

  { &avc_ts_mp_hd_aac_ltp_mult7, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },
  { &avc_ts_mp_hd_aac_ltp_mult7_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },
  { &avc_ts_mp_hd_aac_ltp_mult7_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_MP_HD, AVC_AUDIO_PROFILE_AAC_LTP_MULT7 },

  { &avc_ts_bl_cif15_aac, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC },
  { &avc_ts_bl_cif15_aac_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC },
  { &avc_ts_bl_cif15_aac_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC },

  { &avc_ts_bl_cif15_aac_540, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_540 },
  { &avc_ts_bl_cif15_aac_540_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_540 },
  { &avc_ts_bl_cif15_aac_540_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_540 },

  { &avc_ts_bl_cif15_aac_ltp, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_bl_cif15_aac_ltp_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_ts_bl_cif15_aac_ltp_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AAC_LTP },

  { &avc_ts_bl_cif15_bsac, CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_BSAC },
  { &avc_ts_bl_cif15_bsac_t, CT_MPEG_TRANSPORT_STREAM_DLNA,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_BSAC },
  { &avc_ts_bl_cif15_bsac_iso, CT_MPEG_TRANSPORT_STREAM,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_BSAC },
  
  /* 3GPP Container */
  { &avc_3gpp_bl_cif30_amr_wbplus, CT_3GP,
    AVC_VIDEO_PROFILE_BL_CIF30, AVC_AUDIO_PROFILE_AMR_WB },

  { &avc_3gpp_bl_cif15_amr_wbplus, CT_3GP,
    AVC_VIDEO_PROFILE_BL_CIF15, AVC_AUDIO_PROFILE_AMR_WB },

  { &avc_3gpp_bl_qcif15_aac, CT_3GP,
    AVC_VIDEO_PROFILE_BL_QCIF15, AVC_AUDIO_PROFILE_AAC },
  { &avc_3gpp_bl_qcif15_aac_ltp, CT_3GP,
    AVC_VIDEO_PROFILE_BL_QCIF15, AVC_AUDIO_PROFILE_AAC_LTP },
  { &avc_3gpp_bl_qcif15_heaac, CT_3GP,
    AVC_VIDEO_PROFILE_BL_QCIF15, AVC_AUDIO_PROFILE_HEAAC },
  { &avc_3gpp_bl_qcif15_amr_wbplus, CT_3GP,
    AVC_VIDEO_PROFILE_BL_QCIF15, AVC_AUDIO_PROFILE_AMR_WB },
  { &avc_3gpp_bl_qcif15_amr, CT_3GP,
    AVC_VIDEO_PROFILE_BL_QCIF15, AVC_AUDIO_PROFILE_AMR },
  
  { NULL }
};
  
static dlna_profile_t *
probe_avc (AVFormatContext *ctx)
{
  av_codecs_t *codecs;
  dlna_container_type_t st;

  /* grab codecs info */
  codecs = av_profile_get_codecs (ctx);
  if (!codecs)
    goto probe_avc_end;

  /* check for H.264/AVC codec */
  if (codecs->vc->codec_id != CODEC_ID_H264)
    goto probe_avc_end;

  /* check for a supported container */
  st = stream_get_container (ctx);
  if (st != CT_3GP &&
      st != CT_MP4 &&
      st != CT_MPEG_TRANSPORT_STREAM &&
      st != CT_MPEG_TRANSPORT_STREAM_DLNA &&
      st != CT_MPEG_TRANSPORT_STREAM_DLNA_NO_TS)
    goto probe_avc_end;
  
 probe_avc_end:
  if (codecs)
    free (codecs);
  
  return NULL;
}

dlna_registered_profile_t dlna_profile_av_mpeg4_part10 = {
  .id = DLNA_PROFILE_AV_MPEG4_PART10,
  .extensions = "mp4,3gp,3gpp,mpg,mpeg,mpe,mp2t,ts",
  .probe = probe_avc,
  .next = NULL
};
