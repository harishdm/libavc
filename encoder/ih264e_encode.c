/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/

/**
******************************************************************************
* @file
*  ih264e_encode.c
*
* @brief
*  This file contains functions for encoding the input yuv frame in synchronous
*  api mode
*
* @author
*  ittiam
*
* List of Functions
*  - ih264e_join_threads
*  - ih264e_wait_for_thread
*  - ih264e_encode
*
******************************************************************************
*/

/*****************************************************************************/
/* File Includes                                                             */
/*****************************************************************************/

/* System Include files */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

/* User Include Files */
#include "ih264e_config.h"
#include "ih264_typedefs.h"
#include "iv2.h"
#include "ive2.h"
#include "ithread.h"

#include "ih264_debug.h"
#include "ih264_macros.h"
#include "ih264_error.h"
#include "ih264_defs.h"
#include "ih264_mem_fns.h"
#include "ih264_padding.h"
#include "ih264_structs.h"
#include "ih264_trans_quant_itrans_iquant.h"
#include "ih264_inter_pred_filters.h"
#include "ih264_intra_pred_filters.h"
#include "ih264_deblk_edge_filters.h"
#include "ih264_cabac_tables.h"
#include "ih264_buf_mgr.h"
#include "ih264_list.h"
#include "ih264_dpb_mgr.h"
#include "ih264_platform_macros.h"

#include "ime_defs.h"
#include "ime_distortion_metrics.h"
#include "ime_structs.h"

#include "irc_mem_req_and_acq.h"
#include "irc_cntrl_param.h"
#include "irc_frame_info_collector.h"

#include "ih264e.h"
#include "ih264e_error.h"
#include "ih264e_defs.h"
#include "ih264e_time_stamp.h"
#include "ih264e_rate_control.h"
#include "ih264e_bitstream.h"
#include "ih264e_cabac_structs.h"
#include "ih264e_structs.h"
#include "ih264e_utils.h"
#include "ih264e_encode_header.h"
#include "ih264e_master.h"
#include "ih264e_process.h"
#include "ih264e_fmt_conv.h"
#include "ih264e_statistics.h"
#include "ih264e_trace.h"
#ifdef LOGO_EN
#include "ih264e_ittiam_logo.h"
#endif


#define SEI_BASED_FORCE_IDR 1

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
*******************************************************************************
*
* @brief
*  Initializes the thread pool for multi-threaded encoding.
*
* @par Description:
*  Creates and initializes the thread pool based on the number of configured
*  cores. It spawns worker threads and sets up necessary synchronization
*  mechanisms.
*
* @param[in] ps_codec
*  Pointer to the codec context structure.
*
* @returns  IV_SUCCESS on success, IV_FAIL on failure.
*
*******************************************************************************
*/
WORD32 ih264e_thread_pool_init(codec_t *ps_codec)
{
    /* temp var */
    WORD32 i = 0, ret = 0;

    /* thread pool */
    thread_pool_t *ps_pool = &ps_codec->s_thread_pool;

    /* Return if already initialized */
    if (ps_pool->i4_init_done == 1) return IV_SUCCESS;

    /* initializing thread pool initialization done */
    ps_pool->i4_init_done = 0;

    /* initializing end of stream */
    ps_pool->i4_end_of_stream = 0;

    /* initializing active threads */
    ps_pool->i4_working_threads = 0;

    /* Initialize new frame ready flag */
    ps_pool->i4_has_frame = 0;

    for (i = 1; i < ps_codec->s_cfg.u4_num_cores; i++)
    {
        ret = ithread_create(&ps_pool->apv_threads[i], NULL, ih264e_thread_worker,
                             (void *) &ps_codec->as_process[i]);
        if (ret != 0)
        {
            ps_codec->ai4_process_thread_created[i] = 0;
        }
        else
        {
            ps_codec->ai4_process_thread_created[i] = 1;
            ps_codec->i4_proc_thread_cnt++;
        }
    }

    /* Thread pool initialized */
    ps_pool->i4_init_done = 1;
    return IV_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*  Shuts down the thread pool and cleans up resources.
*
* @par Description:
*  Signals worker threads to terminate, joins all active threads, and
*  destroys associated synchronization primitives.
*
* @param[in] ps_codec
*  Pointer to the codec context structure.
*
* @returns  IV_SUCCESS on success, IV_FAIL on failure.
*
*******************************************************************************
*/
WORD32 ih264e_thread_pool_shutdown(codec_t *ps_codec)
{
    /* thread pool */
    thread_pool_t *ps_pool = &ps_codec->s_thread_pool;

    /* temp var */
    WORD32 i = 0;
    WORD32 ret = IV_SUCCESS;

    /* Wake all threads waiting */
    ithread_mutex_lock(ps_codec->s_thread_pool.pv_thread_pool_mutex);
    ps_pool->i4_end_of_stream = 1;
    ithread_cond_broadcast(ps_codec->s_thread_pool.pv_thread_pool_cond);
    ithread_mutex_unlock(ps_codec->s_thread_pool.pv_thread_pool_mutex);

    /* Join threads */
    for (i = 1; i < ps_codec->s_cfg.u4_num_cores; i++)
    {
        if (ps_codec->ai4_process_thread_created[i])
        {
            if (ithread_join(ps_pool->apv_threads[i], NULL) != 0)
            {
                ret = IV_FAIL;
            }
            else
            {
                ps_codec->ai4_process_thread_created[i] = 0;
                ps_codec->i4_proc_thread_cnt--;
            }
        }
    }

    /* Reset all thread pool state variables */
    ps_pool->i4_init_done = 0;
    ps_pool->i4_end_of_stream = 0;
    ps_pool->i4_working_threads = 0;
    ps_pool->i4_has_frame = 0;

    return ret;
}

/**
*******************************************************************************
*
* @brief
*  Worker thread function for processing encoding tasks.
*
* @par Description:
*  Waits for available jobs and processes encoding tasks until signaled to
*  terminate.
*
* @param[in] pv_proc
*  Pointer to the process context.
*
* @returns  IH264_SUCCESS on completion.
*
*******************************************************************************
*/
static WORD32 ih264e_thread_worker(void *pv_proc)
{
    /* process ctxt */
    process_ctxt_t *ps_proc = (process_ctxt_t *)pv_proc;

    /* process ctxt */
    codec_t *ps_codec = ps_proc->ps_codec;

    /* thread pool ctxt */
    thread_pool_t *ps_pool = &ps_codec->s_thread_pool;

    while (1)
    {
        // Wait until a frame is ready or end of stream
        ithread_mutex_lock(ps_pool->pv_thread_pool_mutex);
        while (!ps_pool->i4_has_frame && !ps_pool->i4_end_of_stream)
        {
            ithread_cond_wait(ps_pool->pv_thread_pool_cond,
                              ps_pool->pv_thread_pool_mutex);
        }

        if (ps_pool->i4_end_of_stream)
        {
            ithread_mutex_unlock(ps_pool->pv_thread_pool_mutex);
            break;
        }

        /* incrementing active threads */
        ps_pool->i4_working_threads++;
        ithread_mutex_unlock(ps_pool->pv_thread_pool_mutex);

        /* worker processes available jobs */
        ih264e_process_thread(pv_proc);

        /* decreasing active threads */
        ithread_mutex_lock(ps_pool->pv_thread_pool_mutex);
        ps_pool->i4_working_threads--;

        /* Notify main thread if all workers are done */
        if (ps_pool->i4_working_threads == 0 &&
            ih264_get_job_count_in_list(ps_codec->pv_proc_jobq) == 0 &&
            ih264_get_job_count_in_list(ps_codec->pv_entropy_jobq) == 0)
        {
            ps_pool->i4_has_frame = 0;
            ithread_cond_signal(ps_pool->pv_thread_pool_cond);
        }
        ithread_mutex_unlock(ps_pool->pv_thread_pool_mutex);
    }

    return IH264_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*  Resets the thread pool state for the next encoding frame.
*
* @par Description:
*  Resets the active thread count and signals worker threads to start
*  processing a new frame.
*
* @param[in] ps_codec
*  Pointer to the codec context structure.
*
* @returns  IH264_SUCCESS on success.
*
*******************************************************************************
*/
WORD32 ih264e_thread_pool_activate(codec_t *ps_codec)
{
    IH264_ERROR_T ret = IH264_SUCCESS;

    /* thread pool ctxt */
    thread_pool_t *ps_pool = &ps_codec->s_thread_pool;

    if (ps_codec->i4_proc_thread_cnt == 0)
    {
        return ret;
    }

    /* reset working threads and new frame */
    ithread_mutex_lock(ps_pool->pv_thread_pool_mutex);
    ps_pool->i4_working_threads = 0;
    ps_pool->i4_has_frame = 1;
    ithread_cond_broadcast(ps_pool->pv_thread_pool_cond);
    ithread_mutex_unlock(ps_pool->pv_thread_pool_mutex);

    return ret;
}

/**
*******************************************************************************
*
* @brief
*  Synchronizes the thread pool by waiting for all tasks to complete.
*
* @par Description:
*  Ensures that all worker threads complete their processing before
*  proceeding to the next frame.
*
* @param[in] ps_codec
*  Pointer to the codec context structure.
*
* @returns  IH264_SUCCESS on success.
*
*******************************************************************************
*/
WORD32 ih264e_thread_pool_sync(codec_t *ps_codec)
{
    IH264_ERROR_T ret = IH264_SUCCESS;

    /* thread pool ctxt */
    thread_pool_t *ps_pool = &ps_codec->s_thread_pool;

    /* skip for single thread */
    if (ps_codec->i4_proc_thread_cnt == 0)
    {
        return ret;
    }

    // Wait for workers to complete
    ithread_mutex_lock(ps_pool->pv_thread_pool_mutex);
    while (ps_pool->i4_has_frame == 1)
    {
        ithread_cond_wait(ps_pool->pv_thread_pool_cond,
                          ps_pool->pv_thread_pool_mutex);
    }
    ithread_mutex_unlock(ps_pool->pv_thread_pool_mutex);

    return ret;
}

/**
******************************************************************************
*
* @brief
*  This function joins all the spawned threads after successful completion of
*  their tasks
*
* @par   Description
*
* @param[in] ps_codec
*  pointer to codec context
*
* @returns  none
*
******************************************************************************
*/
void ih264e_join_threads(codec_t *ps_codec)
{
    /* temp var */
   WORD32 i = 0;
   WORD32 ret = 0;

   /* join spawned threads */
   while (i < ps_codec->i4_proc_thread_cnt)
   {
       if (ps_codec->ai4_process_thread_created[i])
       {
           ret = ithread_join(ps_codec->apv_proc_thread_handle[i], NULL);
           if (ret != 0)
           {
               printf("pthread Join Failed");
               assert(0);
           }
           ps_codec->ai4_process_thread_created[i] = 0;
           i++;
       }
   }

   ps_codec->i4_proc_thread_cnt = 0;
}

/**
******************************************************************************
*
* @brief This function puts the current thread to sleep for a duration
*  of sleep_us
*
* @par Description
*  ithread_yield() method causes the calling thread to yield execution to another
*  thread that is ready to run on the current processor. The operating system
*  selects the thread to yield to. ithread_usleep blocks the current thread for
*  the specified number of milliseconds. In other words, yield just says,
*  end my timeslice prematurely, look around for other threads to run. If there
*  is nothing better than me, continue. Sleep says I don't want to run for x
*  milliseconds. Even if no other thread wants to run, don't make me run.
*
* @param[in] sleep_us
*  thread sleep duration
*
* @returns error_status
*
******************************************************************************
*/
IH264E_ERROR_T ih264e_wait_for_thread(UWORD32 sleep_us)
{
    /* yield thread */
    ithread_yield();

    /* put thread to sleep */
    ithread_usleep(sleep_us);

    return IH264E_SUCCESS;
}

/**
*******************************************************************************
*
* @brief
*  Used to test validity of input dimensions
*
* @par Description:
*  Dimensions of the input buffer passed to encode call are validated
*
* @param[in] ps_codec
*  Codec context
*
* @param[in] ps_ip
*  Pointer to input structure
*
* @param[out] ps_op
*  Pointer to output structure
*
* @returns error status
*
* @remarks none
*
*******************************************************************************
*/
static IV_STATUS_T api_check_input_dimensions(codec_t *ps_codec,
                                              ih264e_video_encode_ip_t *ps_ip,
                                              ih264e_video_encode_op_t *ps_op)
{
    UWORD32 u4_wd, u4_ht;
    cfg_params_t *ps_curr_cfg = &ps_codec->s_cfg;
    iv_raw_buf_t *ps_inp_buf = &ps_ip->s_ive_ip.s_inp_buf;

    u4_wd = ps_inp_buf->au4_wd[0];
    u4_ht = ps_inp_buf->au4_ht[0];
    switch (ps_inp_buf->e_color_fmt)
    {
        case IV_YUV_420P:
            if (((ps_inp_buf->au4_wd[0] / 2) != ps_inp_buf->au4_wd[1]) ||
                            ((ps_inp_buf->au4_wd[0] / 2) != ps_inp_buf->au4_wd[2]) ||
                            (ps_inp_buf->au4_wd[1] != ps_inp_buf->au4_wd[2]))
            {
                ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
                ps_op->s_ive_op.u4_error_code |= IH264E_WIDTH_NOT_SUPPORTED;
                return (IV_FAIL);
            }
            if (((ps_inp_buf->au4_ht[0] / 2) != ps_inp_buf->au4_ht[1]) ||
                            ((ps_inp_buf->au4_ht[0] / 2) != ps_inp_buf->au4_ht[2]) ||
                            (ps_inp_buf->au4_ht[1] != ps_inp_buf->au4_ht[2]))
            {
                ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
                ps_op->s_ive_op.u4_error_code |= IH264E_HEIGHT_NOT_SUPPORTED;
                return (IV_FAIL);
            }
            break;
        case IV_YUV_420SP_UV:
        case IV_YUV_420SP_VU:
            if (ps_inp_buf->au4_wd[0] != ps_inp_buf->au4_wd[1])
            {
                ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
                ps_op->s_ive_op.u4_error_code |= IH264E_WIDTH_NOT_SUPPORTED;
                return (IV_FAIL);
            }
            if ((ps_inp_buf->au4_ht[0] / 2) != ps_inp_buf->au4_ht[1])
            {
                ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
                ps_op->s_ive_op.u4_error_code |= IH264E_HEIGHT_NOT_SUPPORTED;
                return (IV_FAIL);
            }
            break;
        case IV_YUV_422ILE:
            u4_wd = ps_inp_buf->au4_wd[0] / 2;
            break;
        default:
            ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
            ps_op->s_ive_op.u4_error_code |= IH264E_INPUT_CHROMA_FORMAT_NOT_SUPPORTED;
            return (IV_FAIL);
    }

    if (u4_wd != ps_curr_cfg->u4_disp_wd)
    {
        ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
        ps_op->s_ive_op.u4_error_code |= IH264E_WIDTH_NOT_SUPPORTED;
        return (IV_FAIL);
    }

    if (u4_ht != ps_curr_cfg->u4_disp_ht)
    {
        ps_op->s_ive_op.u4_error_code |= 1 << IVE_UNSUPPORTEDPARAM;
        ps_op->s_ive_op.u4_error_code |= IH264E_HEIGHT_NOT_SUPPORTED;
        return (IV_FAIL);
    }

    return IV_SUCCESS;
}

/**
******************************************************************************
*
* @brief
*  Encodes in synchronous api mode
*
* @par Description
*  This routine processes input yuv, encodes it and outputs bitstream and recon
*
* @param[in] ps_codec_obj
*  Pointer to codec object at API level
*
* @param[in] pv_api_ip
*  Pointer to input argument structure
*
* @param[out] pv_api_op
*  Pointer to output argument structure
*
* @returns  Status
*
******************************************************************************
*/
WORD32 ih264e_encode(iv_obj_t *ps_codec_obj, void *pv_api_ip, void *pv_api_op)
{
    /* error status */
    IH264E_ERROR_T error_status = IH264E_SUCCESS;

    /* codec ctxt */
    codec_t *ps_codec = (codec_t *)ps_codec_obj->pv_codec_handle;

    /* input frame to encode */
    ih264e_video_encode_ip_t *ps_video_encode_ip = pv_api_ip;

    /* output buffer to write stream */
    ih264e_video_encode_op_t *ps_video_encode_op = pv_api_op;

    /* i/o structures */
    inp_buf_t s_inp_buf = {};
    out_buf_t s_out_buf = {};

    /* temp var */
    WORD32 ctxt_sel = 0, i, i4_rc_pre_enc_skip;

    /********************************************************************/
    /*                            BEGIN INIT                            */
    /********************************************************************/
    /* reset output structure */
    ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;
    ps_video_encode_op->s_ive_op.output_present  = 0;
    ps_video_encode_op->s_ive_op.dump_recon = 0;
    ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_NA_FRAME;
    /* By default set the current input buffer as the buffer to be freed */
    /* This will later be updated to the actual input that gets encoded */
    ps_video_encode_op->s_ive_op.s_inp_buf = ps_video_encode_ip->s_ive_ip.s_inp_buf;

    if (ps_codec->i4_error_code & (1 << IVE_FATALERROR))
    {
        error_status = ps_codec->i4_error_code & 0xFF;
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);
    }

    /* Check for output memory allocation size */
    if (ps_video_encode_ip->s_ive_ip.s_out_buf.u4_bufsize < MIN_STREAM_SIZE)
    {
        error_status = IH264E_INSUFFICIENT_OUTPUT_BUFFER;
        SET_ERROR_ON_RETURN(error_status,
                            IVE_UNSUPPORTEDPARAM,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);
    }

    if (ps_codec->i4_init_done != 1)
    {
        error_status = IH264E_INIT_NOT_DONE;
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);
    }

    /* copy output info. to internal structure */
    s_out_buf.s_bits_buf = ps_video_encode_ip->s_ive_ip.s_out_buf;
    s_out_buf.u4_is_last = 0;
    s_out_buf.u4_timestamp_low = ps_video_encode_ip->s_ive_ip.u4_timestamp_low;
    s_out_buf.u4_timestamp_high = ps_video_encode_ip->s_ive_ip.u4_timestamp_high;

    /* api call cnt */
    ps_codec->i4_encode_api_call_cnt += 1;

    /* codec context selector */
    ctxt_sel = ps_codec->i4_encode_api_call_cnt % MAX_CTXT_SETS;

    /* reset status flags */
    ps_codec->ai4_pic_cnt[ctxt_sel] = -1;
    ps_codec->s_rate_control.post_encode_skip[ctxt_sel] = 0;
    ps_codec->s_rate_control.pre_encode_skip[ctxt_sel] = 0;

    /* pass output buffer to codec */
    ps_codec->as_out_buf[ctxt_sel] = s_out_buf;

    /* initialize codec ctxt with default params for the first encode api call */
    if (ps_codec->i4_encode_api_call_cnt == 0)
    {
        ih264e_codec_init(ps_codec);
    }

    /* parse configuration params */
    for (i = 0; i < MAX_ACTIVE_CONFIG_PARAMS; i++)
    {
        cfg_params_t *ps_cfg = &ps_codec->as_cfg[i];

        if (1 == ps_cfg->u4_is_valid)
        {
            if ( ((ps_cfg->u4_timestamp_high == ps_video_encode_ip->s_ive_ip.u4_timestamp_high) &&
                            (ps_cfg->u4_timestamp_low == ps_video_encode_ip->s_ive_ip.u4_timestamp_low)) ||
                            ((WORD32)ps_cfg->u4_timestamp_high == -1) ||
                            ((WORD32)ps_cfg->u4_timestamp_low == -1) )
            {
                error_status = ih264e_codec_update_config(ps_codec, ps_cfg);
                SET_ERROR_ON_RETURN(error_status,
                                    IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);

                ps_cfg->u4_is_valid = 0;
            }
        }
    }
    /* Force IDR based on SEI params */
#if SEI_BASED_FORCE_IDR
    {
        int i;
        bool au4_sub_layer_num_units_in_shutter_interval_flag = 0;

        sei_mdcv_params_t *ps_sei_mdcv_params = &ps_codec->s_sei.s_sei_mdcv_params;
        sei_mdcv_params_t *ps_cfg_sei_mdcv_params =
                                &ps_codec->s_cfg.s_sei.s_sei_mdcv_params;
        sei_cll_params_t *ps_sei_cll_params = &ps_codec->s_sei.s_sei_cll_params;
        sei_cll_params_t *ps_cfg_sei_cll_params =
                                &ps_codec->s_cfg.s_sei.s_sei_cll_params;
        sei_ave_params_t *ps_sei_ave_params = &ps_codec->s_sei.s_sei_ave_params;
        sei_ave_params_t *ps_cfg_sei_ave_params =
                                &ps_codec->s_cfg.s_sei.s_sei_ave_params;
        sei_sii_params_t *ps_sei_sii_params = &ps_codec->s_sei.s_sei_sii_params;
        sei_sii_params_t *ps_cfg_sei_sii_params = &ps_codec->s_cfg.s_sei.s_sei_sii_params;

        if((ps_sei_mdcv_params->au2_display_primaries_x[0]!=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_x[0]) ||
            (ps_sei_mdcv_params->au2_display_primaries_x[1] !=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_x[1]) ||
            (ps_sei_mdcv_params->au2_display_primaries_x[2] !=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_x[2]) ||
            (ps_sei_mdcv_params->au2_display_primaries_y[0] !=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_y[0]) ||
            (ps_sei_mdcv_params->au2_display_primaries_y[1] !=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_y[1]) ||
            (ps_sei_mdcv_params->au2_display_primaries_y[2] !=
                                ps_cfg_sei_mdcv_params->au2_display_primaries_y[2]) ||
            (ps_sei_mdcv_params->u2_white_point_x !=
                                ps_cfg_sei_mdcv_params->u2_white_point_x) ||
            (ps_sei_mdcv_params->u2_white_point_y !=
                                ps_cfg_sei_mdcv_params->u2_white_point_y) ||
            (ps_sei_mdcv_params->u4_max_display_mastering_luminance !=
                                ps_cfg_sei_mdcv_params->u4_max_display_mastering_luminance) ||
            (ps_sei_mdcv_params->u4_min_display_mastering_luminance !=
                                ps_cfg_sei_mdcv_params->u4_min_display_mastering_luminance))
        {
            ps_codec->s_sei.s_sei_mdcv_params = ps_codec->s_cfg.s_sei.s_sei_mdcv_params;
            ps_codec->s_sei.u1_sei_mdcv_params_present_flag = 1;
        }
        else
        {
            ps_codec->s_sei.u1_sei_mdcv_params_present_flag = 0;
        }

        if((ps_sei_cll_params->u2_max_content_light_level !=
                                ps_cfg_sei_cll_params->u2_max_content_light_level) ||
                (ps_sei_cll_params->u2_max_pic_average_light_level !=
                                ps_cfg_sei_cll_params->u2_max_pic_average_light_level))
        {
            ps_codec->s_sei.s_sei_cll_params = ps_codec->s_cfg.s_sei.s_sei_cll_params;
            ps_codec->s_sei.u1_sei_cll_params_present_flag = 1;
        }
        else
        {
            ps_codec->s_sei.u1_sei_cll_params_present_flag = 0;
        }

        if((ps_sei_ave_params->u4_ambient_illuminance !=
                                ps_cfg_sei_ave_params->u4_ambient_illuminance) ||
                (ps_sei_ave_params->u2_ambient_light_x !=
                                ps_cfg_sei_ave_params->u2_ambient_light_x) ||
                (ps_sei_ave_params->u2_ambient_light_y !=
                                ps_cfg_sei_ave_params->u2_ambient_light_y))
        {
            ps_codec->s_sei.s_sei_ave_params = ps_codec->s_cfg.s_sei.s_sei_ave_params;
            ps_codec->s_sei.u1_sei_ave_params_present_flag = 1;
        }
        else
        {
            ps_codec->s_sei.u1_sei_ave_params_present_flag = 0;
        }

        for(i = 0; i <= ps_cfg_sei_sii_params->u1_sii_max_sub_layers_minus1; i++)
        {
            au4_sub_layer_num_units_in_shutter_interval_flag =
                (au4_sub_layer_num_units_in_shutter_interval_flag ||
                 (ps_sei_sii_params->au4_sub_layer_num_units_in_shutter_interval[i] !=
                  ps_cfg_sei_sii_params->au4_sub_layer_num_units_in_shutter_interval[i]));
        }

        if((ps_sei_sii_params->u4_sii_sub_layer_idx !=
            ps_cfg_sei_sii_params->u4_sii_sub_layer_idx) ||
           (ps_sei_sii_params->u1_shutter_interval_info_present_flag !=
            ps_cfg_sei_sii_params->u1_shutter_interval_info_present_flag) ||
           (ps_sei_sii_params->u4_sii_time_scale != ps_cfg_sei_sii_params->u4_sii_time_scale) ||
           (ps_sei_sii_params->u1_fixed_shutter_interval_within_cvs_flag !=
            ps_cfg_sei_sii_params->u1_fixed_shutter_interval_within_cvs_flag) ||
           (ps_sei_sii_params->u4_sii_num_units_in_shutter_interval !=
            ps_cfg_sei_sii_params->u4_sii_num_units_in_shutter_interval) ||
           (ps_sei_sii_params->u1_sii_max_sub_layers_minus1 !=
            ps_cfg_sei_sii_params->u1_sii_max_sub_layers_minus1) ||
           au4_sub_layer_num_units_in_shutter_interval_flag)
        {
            ps_codec->s_sei.s_sei_sii_params = ps_codec->s_cfg.s_sei.s_sei_sii_params;
            ps_codec->s_sei.u1_sei_sii_params_present_flag = 1;
        }
        else
        {
            ps_codec->s_sei.u1_sei_sii_params_present_flag = 0;
        }

        if((1 == ps_codec->s_sei.u1_sei_mdcv_params_present_flag) ||
                (1 == ps_codec->s_sei.u1_sei_cll_params_present_flag) ||
           (1 == ps_codec->s_sei.u1_sei_ave_params_present_flag) ||
           (1 == ps_codec->s_sei.u1_sei_sii_params_present_flag))
        {
            ps_codec->force_curr_frame_type = IV_IDR_FRAME;
        }
    }
#endif

    if (ps_video_encode_ip->s_ive_ip.s_inp_buf.apv_bufs[0] != NULL &&
                    ps_codec->i4_header_mode != 1)
    {
        if (IV_SUCCESS != api_check_input_dimensions(ps_codec, pv_api_ip, pv_api_op))
        {
            error_status = IH264E_FAIL;
            SET_ERROR_ON_RETURN(error_status,
                                IVE_FATALERROR,
                                ps_video_encode_op->s_ive_op.u4_error_code,
                                IV_FAIL);
        }
        /******************************************************************
         * INSERT LOGO
         *****************************************************************/
#ifdef LOGO_EN
        ih264e_insert_logo(ps_video_encode_ip->s_ive_ip.s_inp_buf.apv_bufs[0],
                           ps_video_encode_ip->s_ive_ip.s_inp_buf.apv_bufs[1],
                           ps_video_encode_ip->s_ive_ip.s_inp_buf.apv_bufs[2],
                           ps_video_encode_ip->s_ive_ip.s_inp_buf.au4_strd[0],
                           0,
                           0,
                           ps_codec->s_cfg.e_inp_color_fmt,
                           ps_codec->s_cfg.u4_disp_wd,
                           ps_codec->s_cfg.u4_disp_ht);
#endif /*LOGO_EN*/
    }

    /* In case of alt ref and B pics we will have non reference frame in stream */
    if (ps_codec->s_cfg.u4_enable_alt_ref || ps_codec->s_cfg.u4_num_bframes)
    {
        ps_codec->i4_non_ref_frames_in_stream = 1;
    }

    if (ps_codec->i4_encode_api_call_cnt == 0)
    {
        /********************************************************************/
        /*   number of mv/ref bank buffers used by the codec,               */
        /*      1 to handle curr frame                                      */
        /*      1 to store information of ref frame                         */
        /*      1 more additional because of the codec employs 2 ctxt sets  */
        /*        to assist asynchronous API                                */
        /********************************************************************/

        /* initialize mv bank buffer manager */
        error_status = ih264e_mv_buf_mgr_add_bufs(ps_codec);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* initialize ref bank buffer manager */
        error_status = ih264e_pic_buf_mgr_add_bufs(ps_codec);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* for the first frame, generate header when not requested explicitly */
        if (ps_codec->i4_header_mode == 0 &&
                        ps_codec->u4_header_generated == 0)
        {
            ps_codec->i4_gen_header = 1;
        }

        if (ps_codec->s_cfg.u4_keep_threads_active)
        {
            ih264e_thread_pool_init(ps_codec);
        }
    }

    /* generate header and return when encoder is operated in header mode */
    if (ps_codec->i4_header_mode == 1)
    {
        /* whenever the header is generated, this implies a start of sequence
         * and a sequence needs to be started with IDR
         */
        ps_codec->force_curr_frame_type = IV_IDR_FRAME;

        /* generate header */
        error_status = ih264e_generate_sps_pps(ps_codec);

        /* send the input to app */
        ps_video_encode_op->s_ive_op.s_inp_buf = ps_video_encode_ip->s_ive_ip.s_inp_buf;
        ps_video_encode_op->s_ive_op.u4_timestamp_low = ps_video_encode_ip->s_ive_ip.u4_timestamp_low;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = ps_video_encode_ip->s_ive_ip.u4_timestamp_high;

        ps_video_encode_op->s_ive_op.u4_is_last = ps_video_encode_ip->s_ive_ip.u4_is_last;

        /* send the output to app */
        ps_video_encode_op->s_ive_op.output_present  = 1;
        ps_video_encode_op->s_ive_op.dump_recon = 0;
        ps_video_encode_op->s_ive_op.s_out_buf = ps_codec->as_out_buf[ctxt_sel].s_bits_buf;

        /* error status */
        SET_ERROR_ON_RETURN(error_status,
                            IVE_UNSUPPORTEDPARAM,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        /* indicates that header has been generated previously */
        ps_codec->u4_header_generated = 1;

        /* api call cnt */
        ps_codec->i4_encode_api_call_cnt --;

        /* header mode tag is not sticky */
        ps_codec->i4_header_mode = 0;
        ps_codec->i4_gen_header = 0;

        return IV_SUCCESS;
    }

    /* curr pic cnt */
    ps_codec->i4_pic_cnt += 1;

    i4_rc_pre_enc_skip = 0;
    i4_rc_pre_enc_skip = ih264e_input_queue_update(
                    ps_codec, &ps_video_encode_ip->s_ive_ip, &s_inp_buf);

    s_out_buf.u4_is_last = s_inp_buf.u4_is_last;
    ps_video_encode_op->s_ive_op.u4_is_last = s_inp_buf.u4_is_last;

    /* Send the input to application so that it can free it */
    ps_video_encode_op->s_ive_op.s_inp_buf = s_inp_buf.s_raw_buf;

    /* Only encode if the current frame is not pre-encode skip */
    if (!i4_rc_pre_enc_skip && s_inp_buf.s_raw_buf.apv_bufs[0])
    {
        /* proc ctxt base idx */
        WORD32 proc_ctxt_select = ctxt_sel * MAX_PROCESS_THREADS;

        /* proc ctxt */
        process_ctxt_t *ps_proc = &ps_codec->as_process[proc_ctxt_select];

        WORD32 ret = 0;

        /* number of addl. threads to be created */
        WORD32 num_thread_cnt = ps_codec->s_cfg.u4_num_cores - 1;

        /* array giving pic cnt that is being processed in curr context set */
        ps_codec->ai4_pic_cnt[ctxt_sel] = ps_codec->i4_pic_cnt;

        /* initialize all relevant process ctxts */
        error_status = ih264e_pic_init(ps_codec, &s_inp_buf);
        SET_ERROR_ON_RETURN(error_status,
                            IVE_FATALERROR,
                            ps_video_encode_op->s_ive_op.u4_error_code,
                            IV_FAIL);

        if (ps_codec->s_cfg.u4_keep_threads_active)
        {
            /* reset thread pool and prepare for new frame */
            ih264e_thread_pool_activate(ps_codec);

            /* main thread */
            ih264e_process_thread(&ps_codec->as_process[0]);

            /* sync all threads */
            ih264e_thread_pool_sync(ps_codec);
        }
        else
        {
            for (i = 0; i < num_thread_cnt; i++)
            {
                ret = ithread_create(ps_codec->apv_proc_thread_handle[i],
                                     NULL,
                                     (void *)ih264e_process_thread,
                                     &ps_codec->as_process[i + 1]);
                if (ret != 0)
                {
                    printf("pthread Create Failed");
                    assert(0);
                }

                ps_codec->ai4_process_thread_created[i] = 1;

                ps_codec->i4_proc_thread_cnt++;
            }

            /* launch job */
            ih264e_process_thread(ps_proc);

            /* Join threads at the end of encoding a frame */
            ih264e_join_threads(ps_codec);
        }

        ih264_list_reset(ps_codec->pv_proc_jobq);

        ih264_list_reset(ps_codec->pv_entropy_jobq);

        error_status = ih264e_update_rc_post_enc(ps_codec, ctxt_sel, (ps_codec->i4_poc == 0));
        SET_ERROR_ON_RETURN(error_status,
                            ((error_status == IH264E_BITSTREAM_BUFFER_OVERFLOW) ?
                                            IVE_UNSUPPORTEDPARAM : IVE_FATALERROR),
                            ps_video_encode_op->s_ive_op.u4_error_code, IV_FAIL);

        if (ps_codec->s_cfg.u4_enable_quality_metrics & QUALITY_MASK_PSNR)
        {
            ih264e_compute_quality_stats(ps_proc);
        }

    }

   /****************************************************************************
   * RECON
   *    Since we have forward dependent frames, we cannot return recon in encoding
   *    order. It must be in poc order, or input pic order. To achieve this we
   *    introduce a delay of 1 to the recon wrt encode. Now since we have that
   *    delay, at any point minimum of pic_cnt in our ref buffer will be the
   *    correct frame. For ex let our GOP be IBBP [1 2 3 4] . The encode order
   *    will be [1 4 2 3] .Now since we have a delay of 1, when we are done with
   *    encoding 4, the min in the list will be 1. After encoding 2, it will be
   *    2, 3 after 3 and 4 after 4. Hence we can return in sequence. Note
   *    that the 1 delay is critical. Hence if we have post enc skip, we must
   *    skip here too. Note that since post enc skip already frees the recon
   *    buffer we need not do any thing here
   *
   *    We need to return a recon when ever we consume an input buffer. This
   *    comsumption include a pre or post enc skip. Thus dump recon is set for
   *    all cases except when
   *    1) We are waiting -> ps_codec->i4_pic_cnt > ps_codec->s_cfg.u4_num_bframe
   *        An exception need to be made for the case when we have the last buffer
   *        since we need to flush out the on remainig recon.
   ****************************************************************************/

    ps_video_encode_op->s_ive_op.dump_recon = 0;

    if (ps_codec->s_cfg.u4_enable_recon
                    && (ps_codec->i4_pic_cnt > (WORD32)ps_codec->s_cfg.u4_num_bframes ||
                        s_inp_buf.u4_is_last))
    {
        /* error status */
        IH264_ERROR_T ret = IH264_SUCCESS;
        pic_buf_t *ps_pic_buf = NULL;
        WORD32 i4_buf_status, i4_curr_poc = 32768;
        WORD8 buf_idx = -1;

        /* In case of skips we return recon, but indicate that buffer is zero size */
        if (ps_codec->s_rate_control.post_encode_skip[ctxt_sel]
                        || i4_rc_pre_enc_skip)
        {

            ps_video_encode_op->s_ive_op.dump_recon = 1;
            ps_video_encode_op->s_ive_op.s_recon_buf.au4_wd[0] = 0;
            ps_video_encode_op->s_ive_op.s_recon_buf.au4_wd[1] = 0;

        }
        else
        {
            for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
            {
                if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
                    continue;

                i4_buf_status = ih264_buf_mgr_get_status(
                                ps_codec->pv_ref_buf_mgr,
                                ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);

                if ((i4_buf_status & BUF_MGR_IO)
                                && (ps_codec->as_ref_set[i].i4_poc < i4_curr_poc))
                {
                    ps_pic_buf = ps_codec->as_ref_set[i].ps_pic_buf;
                    i4_curr_poc = ps_codec->as_ref_set[i].i4_poc;
                    buf_idx = i;
                }
            }
            if ((ps_codec->s_cfg.u4_enable_quality_metrics & QUALITY_MASK_PSNR)
                                && buf_idx >= 0)
            {
                UWORD8 comp;
                for(comp = 0; comp < 3; comp++)
                {
                    DEBUG("PSNR[%d]: %f\n", comp,
                        ps_codec->as_ref_set[buf_idx].s_pic_quality_stats.total_psnr[comp]);
                }
            }

            ps_video_encode_op->s_ive_op.s_recon_buf =
                            ps_video_encode_ip->s_ive_ip.s_recon_buf;

            /*
             * If we get a valid buffer. output and free recon.
             *
             * we may get an invalid buffer if num_b_frames is 0. This is because
             * We assume that there will be a ref frame in ref list after encoding
             * the last frame. With B frames this is correct since its forward ref
             * pic will be in the ref list. But if num_b_frames is 0, we will not
             * have a forward ref pic
             */

            if (ps_pic_buf)
            {
                /* copy/convert the recon buffer and return */
                ih264e_fmt_conv(ps_codec,
                                ps_pic_buf,
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[0],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[1],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.apv_bufs[2],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.au4_wd[0],
                                ps_video_encode_ip->s_ive_ip.s_recon_buf.au4_wd[1],
                                0, ps_codec->s_cfg.u4_disp_ht);

                ps_video_encode_op->s_ive_op.dump_recon = 1;

                ret = ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr,
                                            ps_pic_buf->i4_buf_id, BUF_MGR_IO);

                if (IH264_SUCCESS != ret)
                {
                    SET_ERROR_ON_RETURN(
                                    (IH264E_ERROR_T)ret, IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);
                }
            }
        }
    }

    /***************************************************************************
     * Free reference buffers:
     * In case of a post enc skip, we have to ensure that those pics will not
     * be used as reference anymore. In all other cases we will not even mark
     * the ref buffers
     ***************************************************************************/
    if (ps_codec->s_rate_control.post_encode_skip[ctxt_sel])
    {
        /* pic info */
        pic_buf_t *ps_cur_pic;

        /* mv info */
        mv_buf_t *ps_cur_mv_buf;

        /* error status */
        IH264_ERROR_T ret = IH264_SUCCESS;

        /* Decrement coded pic count */
        ps_codec->i4_poc--;

        /* loop through to get the min pic cnt among the list of pics stored in ref list */
        /* since the skipped frame may not be on reference list, we may not have an MV bank
         * hence free only if we have allocated */
        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->i4_pic_cnt == ps_codec->as_ref_set[i].i4_pic_cnt)
            {

                ps_cur_pic = ps_codec->as_ref_set[i].ps_pic_buf;

                ps_cur_mv_buf = ps_codec->as_ref_set[i].ps_mv_buf;

                /* release this frame from reference list and recon list */
                ret = ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr, ps_cur_mv_buf->i4_buf_id , BUF_MGR_REF);
                ret |= ih264_buf_mgr_release(ps_codec->pv_mv_buf_mgr, ps_cur_mv_buf->i4_buf_id , BUF_MGR_IO);
                SET_ERROR_ON_RETURN((IH264E_ERROR_T)ret,
                                    IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);

                ret = ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr, ps_cur_pic->i4_buf_id , BUF_MGR_REF);
                ret |= ih264_buf_mgr_release(ps_codec->pv_ref_buf_mgr, ps_cur_pic->i4_buf_id , BUF_MGR_IO);
                SET_ERROR_ON_RETURN((IH264E_ERROR_T)ret,
                                    IVE_FATALERROR,
                                    ps_video_encode_op->s_ive_op.u4_error_code,
                                    IV_FAIL);
                break;
            }
        }
    }

    /*
     * Since recon is not in sync with output, ie there can be frame to be
     * given back as recon even after last output. Hence we need to mark that
     * the output is not the last.
     * Hence search through reflist and mark appropriately
     */
    if (ps_codec->s_cfg.u4_enable_recon)
    {
        WORD32 i4_buf_status = 0;

        for (i = 0; i < ps_codec->i4_ref_buf_cnt; i++)
        {
            if (ps_codec->as_ref_set[i].i4_pic_cnt == -1)
                continue;

            i4_buf_status |= ih264_buf_mgr_get_status(
                            ps_codec->pv_ref_buf_mgr,
                            ps_codec->as_ref_set[i].ps_pic_buf->i4_buf_id);
        }

        if (i4_buf_status & BUF_MGR_IO)
        {
            s_out_buf.u4_is_last = 0;
            ps_video_encode_op->s_ive_op.u4_is_last = 0;
        }
    }

    /**************************************************************************
     * Signaling to APP
     *  1) If we valid a valid output mark it so
     *  2) Set the codec output ps_video_encode_op
     *  3) Set the error status
     *  4) Set the return Pic type
     *      Note that we already has marked recon properly
     *  5)Send the consumed input back to app so that it can free it if possible
     *
     *  We will have to return the output and input buffers unconditionally
     *  so that app can release them
     **************************************************************************/
    if (!i4_rc_pre_enc_skip
                    && !ps_codec->s_rate_control.post_encode_skip[ctxt_sel]
                    && s_inp_buf.s_raw_buf.apv_bufs[0])
    {

        /* receive output back from codec */
        s_out_buf = ps_codec->as_out_buf[ctxt_sel];

        /* send the output to app */
        ps_video_encode_op->s_ive_op.output_present  = 1;
        ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;

        /* Set the time stamps of the encodec input */
        ps_video_encode_op->s_ive_op.u4_timestamp_low = s_inp_buf.u4_timestamp_low;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = s_inp_buf.u4_timestamp_high;

        switch (ps_codec->pic_type)
        {
            case PIC_IDR:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type =IV_IDR_FRAME;
                break;

            case PIC_I:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_I_FRAME;
                break;

            case PIC_P:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_P_FRAME;
                break;

            case PIC_B:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_B_FRAME;
                break;

            default:
                ps_video_encode_op->s_ive_op.u4_encoded_frame_type = IV_NA_FRAME;
                break;
        }

        for (i = 0; i < (WORD32)ps_codec->s_cfg.u4_num_cores; i++)
        {
            error_status |= ps_codec->as_process[ctxt_sel + i].i4_error_code;
        }
        SET_ERROR_ON_RETURN(error_status,
                            ((error_status == IH264E_BITSTREAM_BUFFER_OVERFLOW) ?
                                            IVE_UNSUPPORTEDPARAM : IVE_FATALERROR),
                            ps_video_encode_op->s_ive_op.u4_error_code, IV_FAIL);
    }
    else
    {
        /* receive output back from codec */
        s_out_buf = ps_codec->as_out_buf[ctxt_sel];

        ps_video_encode_op->s_ive_op.output_present = 0;
        ps_video_encode_op->s_ive_op.u4_error_code = IV_SUCCESS;

        /* Set the time stamps of the encodec input */
        ps_video_encode_op->s_ive_op.u4_timestamp_low = 0;
        ps_video_encode_op->s_ive_op.u4_timestamp_high = 0;

        ps_video_encode_op->s_ive_op.u4_encoded_frame_type =  IV_NA_FRAME;

    }

    ps_video_encode_op->s_ive_op.s_out_buf = s_out_buf.s_bits_buf;

    if (ps_codec->s_cfg.u4_keep_threads_active && ps_video_encode_op->s_ive_op.u4_is_last)
    {
        ih264e_thread_pool_shutdown(ps_codec);
    }

    return IV_SUCCESS;
}
