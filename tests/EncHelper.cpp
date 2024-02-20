/******************************************************************************
 *
 * Copyright (C) 2023 The Android Open Source Project
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

#include "EncHelper.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <map>
#include <string>

#include "BitsBuf.h"
#include "RawBuf.h"
#include "TestCommon.h"
#include "ih264_defs.h"
#include "ih264_typedefs.h"
#include "ih264e.h"
#include "ih264e_error.h"

#define ive_api_function ih264e_api_function

const std::map<std::string, IVE_ENC_MODE_T> kEncMode{{"Picture", IVE_ENC_MODE_PICTURE},
                                                     {"Header", IVE_ENC_MODE_HEADER}};

const std::map<ColorFormat, IV_COLOR_FORMAT_T> kColorFormat{
    {YUV_420P, IV_YUV_420P}, {YUV_420SP_UV, IV_YUV_420SP_UV}, {YUV_420SP_VU, IV_YUV_420SP_VU}};

const std::map<std::string, IV_PICTURE_CODING_TYPE_T> kFrameType{
    {"IDR", IV_IDR_FRAME}, {"I", IV_I_FRAME}, {"P", IV_P_FRAME}, {"B", IV_B_FRAME}};

const std::map<std::string, IVE_RC_MODE_T> kRcMode{{"None", IVE_RC_NONE},
                                                   {"STORAGE", IVE_RC_STORAGE},
                                                   {"CBR_NLD", IVE_RC_CBR_NON_LOW_DELAY},
                                                   {"CBR_LD", IVE_RC_CBR_LOW_DELAY}};

const std::map<std::string, IVE_AIR_MODE_T> kAirMode{
    {"None", IVE_AIR_MODE_NONE}, {"Cyclic", IVE_AIR_MODE_CYCLIC}, {"Random", IVE_AIR_MODE_RANDOM}};

const std::map<std::string, IVE_SPEED_CONFIG> kEncSpeed{
    {"Config", IVE_CONFIG}, {"Slowest", IVE_SLOWEST},      {"Normal", IVE_NORMAL},
    {"Fast", IVE_FAST},     {"HighSpeed", IVE_HIGH_SPEED}, {"Fastest", IVE_FASTEST}};

const std::map<std::string, IV_PROFILE_T> kProfile{{"Baseline", IV_PROFILE_BASE},
                                                   {"Main", IV_PROFILE_MAIN}};

constexpr size_t kHeaderLength = 100000;
EncHelper::EncHelper() { initArgsDefault(); }

EncHelper::~EncHelper() {}

void EncHelper::initArgsDefault() {
    mWidth = 176;
    mHeight = 144;
    mTargetFrameRate = mSourceFrameRate = 30;
    mNumBFrames = 0;
    mMaxWidth = 1920;
    mMaxHeight = 1920;
    mMaxLevel = 51;
    mMaxBitrate = 240000000;
    mMaxFrameRate = 120000;

    mInputYuvFormatKey = YUV_420P;
    mReconYuvFormatKey = YUV_420P;
    mRcModeKey = "STORAGE";
    mAirModeKey = "None";
    mSpeedKey = "Normal";
    mProfileKey = "Main";

    mInputFileName = "";
    mOutputFileName = "";
    mReconFileName = "";
    mSaveOutputFile = false;
    mSaveReconFile = false;

    mInitQpI = 22;
    mInitQpP = 25;
    mInitQpB = 28;

    mMaxQpI = mMaxQpP = mMaxQpB = 51;
    mMinQpI = mMinQpP = mMinQpB = 10;

    mNumFrames = std::numeric_limits<size_t>::max();
    mNumThreads = 1;

    mIntra4x4 = false;
    mConstrainedIntraFlag = false;
    mHalfPelEnable = true;
    mQPelEnable = false;
    mIntraRefresh = false;
    mSliceParam = false;
    mEnableFastSad = false;
    mEnableAltRef = false;
    mConstrainedIntraFlag = false;
    mSeiCllFlag = true;
    mSeiAveFlag = true;
    mSeiCcvFlag = true;
    mSeiMdcvFlag = true;
    mAspectRatioFlag = false;
    mNalHrdFlag = false;
    mVclHrdFlag = false;

    mMeSpeedPreset = 100;
    mSearchRangeX = 64;
    mSearchRangeY = 64;
    mIDRInterval = mIInterval = 30;
    mDisableDeblockLevel = 0;
}

void EncHelper::initArgsRandom() {}

void EncHelper::initArgsFuzzer() {}

void EncHelper::create() {
    mInputFp = fopen(mInputFileName.c_str(), "rb");
    ASSERT_NE(mInputFp, nullptr) << "Failed to open input file: " << mInputFileName;
    mOutputFp = fopen(mOutputFileName.c_str(), "wb");
    ASSERT_NE(mOutputFp, nullptr) << "Failed to open output file: " << mOutputFileName;

    /* Getting Number of MemRecords */
    iv_num_mem_rec_ip_t sNumMemRecIp{};
    iv_num_mem_rec_op_t sNumMemRecOp{};

    sNumMemRecIp.u4_size = sizeof(iv_num_mem_rec_ip_t);
    sNumMemRecOp.u4_size = sizeof(iv_num_mem_rec_op_t);
    sNumMemRecIp.e_cmd = IV_CMD_GET_NUM_MEM_REC;

    IV_STATUS_T status = ive_api_function(nullptr, &sNumMemRecIp, &sNumMemRecOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Error in IV_CMD_GET_NUM_MEM_REC!";

    size_t numMemRecords = sNumMemRecOp.u4_num_mem_rec;
    iv_mem_rec_t *memRecords = (iv_mem_rec_t *)malloc(numMemRecords * sizeof(iv_mem_rec_t));
    ASSERT_NE(memRecords, nullptr) << "Failed to allocate memory to nMemRecords!";
    mMemRecordsBuffer = memRecords;

    iv_mem_rec_t *psMemRec = memRecords;
    for (size_t i = 0; i < numMemRecords; ++i) {
        psMemRec->u4_size = sizeof(iv_mem_rec_t);
        psMemRec->pv_base = nullptr;
        psMemRec->u4_mem_size = 0;
        psMemRec->u4_mem_alignment = 0;
        psMemRec->e_mem_type = IV_NA_MEM_TYPE;
        ++psMemRec;
    }

    /* Getting MemRecords Attributes */
    iv_fill_mem_rec_ip_t sFillMemRecIp{};
    iv_fill_mem_rec_op_t sFillMemRecOp{};

    sFillMemRecIp.u4_size = sizeof(iv_fill_mem_rec_ip_t);
    sFillMemRecOp.u4_size = sizeof(iv_fill_mem_rec_op_t);

    sFillMemRecIp.e_cmd = IV_CMD_FILL_NUM_MEM_REC;
    sFillMemRecIp.ps_mem_rec = memRecords;
    sFillMemRecIp.u4_num_mem_rec = numMemRecords;
    sFillMemRecIp.u4_max_wd = mMaxWidth;
    sFillMemRecIp.u4_max_ht = mMaxHeight;
    sFillMemRecIp.u4_max_level = mMaxLevel;
    sFillMemRecIp.e_color_format = kColorFormat.at(mInputYuvFormatKey);
    sFillMemRecIp.u4_max_ref_cnt = 2;
    sFillMemRecIp.u4_max_reorder_cnt = 0;
    sFillMemRecIp.u4_max_srch_rng_x = 256;
    sFillMemRecIp.u4_max_srch_rng_y = 256;

    status = ive_api_function(nullptr, &sFillMemRecIp, &sFillMemRecOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to fill memory records!: "
                                  << sFillMemRecOp.u4_error_code;

    /* Allocating Memory for Mem Records */
    psMemRec = memRecords;
    for (size_t i = 0; i < numMemRecords; ++i) {
        posix_memalign(&psMemRec->pv_base, psMemRec->u4_mem_alignment, psMemRec->u4_mem_size);
        ASSERT_NE(psMemRec->pv_base, nullptr)
            << "Failed to allocate for size " << psMemRec->u4_mem_size;
        ++psMemRec;
    }

    /* Codec Instance Creation */
    ive_init_ip_t sInitIp{};
    ive_init_op_t sInitOp{};

    iv_obj_t *codecCtx = (iv_obj_t *)memRecords[0].pv_base;
    codecCtx->u4_size = sizeof(iv_obj_t);
    codecCtx->pv_fxns = (void *)ive_api_function;

    sInitIp.u4_size = sizeof(ive_init_ip_t);
    sInitOp.u4_size = sizeof(ive_init_op_t);

    sInitIp.e_cmd = IV_CMD_INIT;
    sInitIp.u4_num_mem_rec = numMemRecords;
    sInitIp.ps_mem_rec = memRecords;
    sInitIp.u4_max_wd = mMaxWidth;
    sInitIp.u4_max_ht = mMaxHeight;
    sInitIp.u4_max_level = mMaxLevel;
    sInitIp.u4_max_ref_cnt = 2;
    sInitIp.u4_max_reorder_cnt = 0;
    sInitIp.e_inp_color_fmt = kColorFormat.at(mInputYuvFormatKey);
    sInitIp.u4_enable_recon = 0;
    sInitIp.e_recon_color_fmt = kColorFormat.at(mReconYuvFormatKey);
    sInitIp.e_rc_mode = kRcMode.at(mRcModeKey);
    sInitIp.u4_max_framerate = mMaxFrameRate;
    sInitIp.u4_max_bitrate = mMaxBitrate;
    sInitIp.u4_num_bframes = mNumBFrames;
    sInitIp.e_content_type = IV_PROGRESSIVE;
    sInitIp.u4_max_srch_rng_x = 256;
    sInitIp.u4_max_srch_rng_y = 256;
    sInitIp.e_slice_mode = IVE_SLICE_MODE_NONE;
    sInitIp.u4_slice_param = 0;
    sInitIp.e_arch = ARCH_NA;
    sInitIp.e_soc = SOC_GENERIC;

    status = ive_api_function(codecCtx, &sInitIp, &sInitOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to create Codec Instance!";

    mCodecCtx = codecCtx;
    mNumMemRecords = numMemRecords;

    ASSERT_NO_FATAL_FAILURE(setDefault());

    ASSERT_NO_FATAL_FAILURE(getBufInfo());

    ASSERT_NO_FATAL_FAILURE(setNumCores());

    ASSERT_NO_FATAL_FAILURE(setDimensions());

    ASSERT_NO_FATAL_FAILURE(setFrameRate());

    ASSERT_NO_FATAL_FAILURE(setIpeParams());

    ASSERT_NO_FATAL_FAILURE(setBitRate());

    ASSERT_NO_FATAL_FAILURE(setQp());

    ASSERT_NO_FATAL_FAILURE(setAirParams());

    ASSERT_NO_FATAL_FAILURE(setVbvParams());

    ASSERT_NO_FATAL_FAILURE(setMeParams());

    ASSERT_NO_FATAL_FAILURE(setGopParams());

    ASSERT_NO_FATAL_FAILURE(setDeblockParams());

    ASSERT_NO_FATAL_FAILURE(setVuiParams());

    ASSERT_NO_FATAL_FAILURE(setSeiMdcvParams());

    ASSERT_NO_FATAL_FAILURE(setSeiCllParams());

    ASSERT_NO_FATAL_FAILURE(setSeiAveParams());

    ASSERT_NO_FATAL_FAILURE(setSeiCcvParams());

    ASSERT_NO_FATAL_FAILURE(setProfileParams());
}

void EncHelper::destroy() {
    iv_mem_rec_t *psMemRec = (iv_mem_rec_t *)mMemRecordsBuffer;

    ih264e_retrieve_mem_rec_ip_t s_retrieve_mem_ip{};
    ih264e_retrieve_mem_rec_op_t s_retrieve_mem_op{};

    s_retrieve_mem_ip.s_ive_ip.u4_size = sizeof(ih264e_retrieve_mem_rec_ip_t);
    s_retrieve_mem_op.s_ive_op.u4_size = sizeof(ih264e_retrieve_mem_rec_op_t);

    s_retrieve_mem_ip.s_ive_ip.e_cmd = IV_CMD_RETRIEVE_MEMREC;
    s_retrieve_mem_ip.s_ive_ip.ps_mem_rec = psMemRec;

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &s_retrieve_mem_ip, &s_retrieve_mem_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to retrieve memory records.";

    for (size_t i = 0; i < mNumMemRecords; ++i) {
        if (psMemRec) {
            free(psMemRec->pv_base);
        }
        ++psMemRec;
    }

    if (mMemRecordsBuffer) {
        free(mMemRecordsBuffer);
        mMemRecordsBuffer = nullptr;
    }

    mCodecCtx = nullptr;

    if (mInputFp)
        fclose(mInputFp);
    if (mOutputFp)
        fclose(mOutputFp);
}

void EncHelper::process() {
    iv_obj_t *codecCtx = (iv_obj_t *)mCodecCtx;

    ASSERT_NO_FATAL_FAILURE(setEncMode("Header"));
    ih264e_video_encode_ip_t ih264e_video_encode_ip{};
    ih264e_video_encode_op_t ih264e_video_encode_op{};

    ive_video_encode_ip_t *sEncodeIp = &ih264e_video_encode_ip.s_ive_ip;
    ive_video_encode_op_t *sEncodeOp = &ih264e_video_encode_op.s_ive_op;

    size_t frameSize = mWidth * mHeight * 3 / 2;
    BitsBuf outputBuf(frameSize);
    outputBuf.alloc();

    iv_raw_buf_t *psInpRawBuf = &sEncodeIp->s_inp_buf;

    sEncodeIp->s_out_buf.pv_buf = outputBuf.mBuffer.data();
    sEncodeIp->s_out_buf.u4_bufsize = outputBuf.mBufferSize;
    sEncodeIp->u4_size = sizeof(ih264e_video_encode_ip_t);
    sEncodeOp->u4_size = sizeof(ih264e_video_encode_op_t);

    sEncodeIp->e_cmd = IVE_CMD_VIDEO_ENCODE;
    sEncodeIp->pv_bufs = nullptr;
    sEncodeIp->pv_mb_info = nullptr;
    sEncodeIp->pv_pic_info = nullptr;
    sEncodeIp->u4_mb_info_type = 0;
    sEncodeIp->u4_pic_info_type = 0;
    sEncodeIp->u4_is_last = 0;
    sEncodeOp->s_out_buf.pv_buf = nullptr;

    psInpRawBuf->u4_size = sizeof(iv_raw_buf_t);
    psInpRawBuf->e_color_fmt = kColorFormat.at(mInputYuvFormatKey);

    IV_STATUS_T status = ive_api_function(codecCtx, sEncodeIp, sEncodeOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to Encode Header\n";

    if (mOutputFp && sEncodeOp->output_present) {
        ASSERT_NE(outputBuf.write(mOutputFp, sEncodeOp->s_out_buf.u4_bytes), false)
            << "Failed to write the output!" << mOutputFileName;
    }

    ASSERT_NO_FATAL_FAILURE(setEncMode("Picture"));
    uint32_t numFrame = 0;
    size_t numFramesToEncode = mNumFrames;

    RawBuf inputBuf(mWidth, mHeight, mWidth, mInputYuvFormatKey);
    inputBuf.alloc();

    while (numFramesToEncode > 0) {
        if (!inputBuf.read(mInputFp)) {
            break;
        }

        psInpRawBuf->e_color_fmt = kColorFormat.at(inputBuf.mFormat);
        for (size_t i = 0; i < kMaxComponents; i++) {
            psInpRawBuf->apv_bufs[i] = (void *)inputBuf.mBuffer[i].data();
            psInpRawBuf->au4_wd[i] = inputBuf.mWidth[i];
            psInpRawBuf->au4_ht[i] = inputBuf.mHeight[i];
            psInpRawBuf->au4_strd[i] = inputBuf.mStride[i];
        }
        sEncodeIp->s_out_buf.pv_buf = outputBuf.mBuffer.data();
        sEncodeIp->s_out_buf.u4_bufsize = outputBuf.mBufferSize;

        status = ive_api_function(codecCtx, &ih264e_video_encode_ip, &ih264e_video_encode_op);
        ASSERT_EQ(status, IV_SUCCESS) << "Failed to encode frame!\n";
        if (mOutputFp && sEncodeOp->output_present) {
            ASSERT_NE(outputBuf.write(mOutputFp, sEncodeOp->s_out_buf.u4_bytes), false)
                << "Failed to write the output!" << mOutputFileName;
        }
        numFramesToEncode--;
        numFrame++;
    }

    sEncodeIp->u4_is_last = 1;
    psInpRawBuf->apv_bufs[0] = nullptr;
    psInpRawBuf->apv_bufs[1] = nullptr;
    psInpRawBuf->apv_bufs[2] = nullptr;

    status = ive_api_function(codecCtx, &ih264e_video_encode_ip, &ih264e_video_encode_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failure after encoding last frame!\n";

    if (mOutputFp && sEncodeOp->output_present) {
        ASSERT_NE(outputBuf.write(mOutputFp, sEncodeOp->s_out_buf.u4_bytes), false)
            << "Failed to write the output!" << mOutputFileName;
    }
}

void EncHelper::setDimensions() {
    ive_ctl_set_dimensions_ip_t sDimensionsIp{};
    ive_ctl_set_dimensions_op_t sDimensionsOp{};

    sDimensionsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sDimensionsIp.e_sub_cmd = IVE_CMD_CTL_SET_DIMENSIONS;
    sDimensionsIp.u4_ht = mHeight;
    sDimensionsIp.u4_wd = mWidth;

    sDimensionsIp.u4_timestamp_high = -1;
    sDimensionsIp.u4_timestamp_low = -1;

    sDimensionsIp.u4_size = sizeof(ive_ctl_set_dimensions_ip_t);
    sDimensionsOp.u4_size = sizeof(ive_ctl_set_dimensions_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sDimensionsIp, &sDimensionsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set dimensions!\n";
}

void EncHelper::setNumCores() {
    ive_ctl_set_num_cores_ip_t sNumCoresIp{};
    ive_ctl_set_num_cores_op_t sNumCoresOp{};

    sNumCoresIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sNumCoresIp.e_sub_cmd = IVE_CMD_CTL_SET_NUM_CORES;
    sNumCoresIp.u4_num_cores = mNumThreads;

    sNumCoresIp.u4_timestamp_high = -1;
    sNumCoresIp.u4_timestamp_low = -1;

    sNumCoresIp.u4_size = sizeof(ive_ctl_set_num_cores_ip_t);
    sNumCoresOp.u4_size = sizeof(ive_ctl_set_num_cores_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, (void *)&sNumCoresIp, (void *)&sNumCoresOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set number of cores!\n";
}

void EncHelper::setDefault() {
    ive_ctl_setdefault_ip_t sDefaultIp{};
    ive_ctl_setdefault_op_t sDefaultOp{};

    sDefaultIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sDefaultIp.e_sub_cmd = IVE_CMD_CTL_SETDEFAULT;

    sDefaultIp.u4_timestamp_high = -1;
    sDefaultIp.u4_timestamp_low = -1;

    sDefaultIp.u4_size = sizeof(ive_ctl_setdefault_ip_t);
    sDefaultOp.u4_size = sizeof(ive_ctl_setdefault_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sDefaultIp, &sDefaultOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set default encoder parameters!\n";
}

void EncHelper::getBufInfo() {
    ih264e_ctl_getbufinfo_ip_t sGetBufInfoIp{};
    ih264e_ctl_getbufinfo_op_t sGetBufInfoOp{};

    sGetBufInfoIp.s_ive_ip.u4_size = sizeof(ih264e_ctl_getbufinfo_ip_t);
    sGetBufInfoOp.s_ive_op.u4_size = sizeof(ih264e_ctl_getbufinfo_op_t);

    sGetBufInfoIp.s_ive_ip.e_cmd = IVE_CMD_VIDEO_CTL;
    sGetBufInfoIp.s_ive_ip.e_sub_cmd = IVE_CMD_CTL_GETBUFINFO;
    sGetBufInfoIp.s_ive_ip.u4_max_ht = mMaxHeight;
    sGetBufInfoIp.s_ive_ip.u4_max_wd = mMaxWidth;
    sGetBufInfoIp.s_ive_ip.e_inp_color_fmt = kColorFormat.at(mInputYuvFormatKey);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sGetBufInfoIp, &sGetBufInfoOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to get buffer info!\n";
}

void EncHelper::setFrameRate() {
    ive_ctl_set_frame_rate_ip_t sFrameRateIp{};
    ive_ctl_set_frame_rate_op_t sFrameRateOp{};

    sFrameRateIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sFrameRateIp.e_sub_cmd = IVE_CMD_CTL_SET_FRAMERATE;
    sFrameRateIp.u4_src_frame_rate = mSourceFrameRate;
    sFrameRateIp.u4_tgt_frame_rate = mTargetFrameRate;

    sFrameRateIp.u4_timestamp_high = -1;
    sFrameRateIp.u4_timestamp_low = -1;

    sFrameRateIp.u4_size = sizeof(ive_ctl_set_frame_rate_ip_t);
    sFrameRateOp.u4_size = sizeof(ive_ctl_set_frame_rate_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sFrameRateIp, &sFrameRateOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set frame rate!\n";
}

void EncHelper::setIpeParams() {
    ive_ctl_set_ipe_params_ip_t sIpeParamsIp{};
    ive_ctl_set_ipe_params_op_t sIpeParamsOp{};

    sIpeParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sIpeParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_IPE_PARAMS;
    sIpeParamsIp.u4_enable_intra_4x4 = mIntra4x4;
    sIpeParamsIp.u4_enc_speed_preset = kEncSpeed.at(mSpeedKey);
    sIpeParamsIp.u4_constrained_intra_pred = mConstrainedIntraFlag;

    sIpeParamsIp.u4_timestamp_high = -1;
    sIpeParamsIp.u4_timestamp_low = -1;

    sIpeParamsIp.u4_size = sizeof(ive_ctl_set_ipe_params_ip_t);
    sIpeParamsOp.u4_size = sizeof(ive_ctl_set_ipe_params_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sIpeParamsIp, &sIpeParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set IPE params!\n";
}

void EncHelper::setBitRate() {
    ive_ctl_set_bitrate_ip_t sBitrateIp{};
    ive_ctl_set_bitrate_op_t sBitrateOp{};

    sBitrateIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sBitrateIp.e_sub_cmd = IVE_CMD_CTL_SET_BITRATE;
    sBitrateIp.u4_target_bitrate = mBitrate;

    sBitrateIp.u4_timestamp_high = -1;
    sBitrateIp.u4_timestamp_low = -1;

    sBitrateIp.u4_size = sizeof(ive_ctl_set_bitrate_ip_t);
    sBitrateOp.u4_size = sizeof(ive_ctl_set_bitrate_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sBitrateIp, &sBitrateOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set bit rate!\n";
}

void EncHelper::setFrameType(std::string frameTypeKey) {
    ive_ctl_set_frame_type_ip_t sFrameTypeIp{};
    ive_ctl_set_frame_type_op_t sFrameTypeOp{};

    sFrameTypeIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sFrameTypeIp.e_sub_cmd = IVE_CMD_CTL_SET_FRAMETYPE;
    sFrameTypeIp.e_frame_type = kFrameType.at(frameTypeKey);

    sFrameTypeIp.u4_timestamp_high = -1;
    sFrameTypeIp.u4_timestamp_low = -1;

    sFrameTypeIp.u4_size = sizeof(ive_ctl_set_frame_type_ip_t);
    sFrameTypeOp.u4_size = sizeof(ive_ctl_set_frame_type_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sFrameTypeIp, &sFrameTypeOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set Frame Type!\n";
}

void EncHelper::setQp() {
    ive_ctl_set_qp_ip_t s_QpIp{};
    ive_ctl_set_qp_op_t s_QpOp{};

    s_QpIp.e_cmd = IVE_CMD_VIDEO_CTL;
    s_QpIp.e_sub_cmd = IVE_CMD_CTL_SET_QP;

    s_QpIp.u4_i_qp = mInitQpI;
    s_QpIp.u4_i_qp_max = mMaxQpI;
    s_QpIp.u4_i_qp_min = mMinQpI;

    s_QpIp.u4_p_qp = mInitQpP;
    s_QpIp.u4_p_qp_max = mMaxQpP;
    s_QpIp.u4_p_qp_min = mMinQpP;

    s_QpIp.u4_b_qp = mInitQpB;
    s_QpIp.u4_b_qp_max = mMaxQpB;
    s_QpIp.u4_b_qp_min = mMinQpB;

    s_QpIp.u4_timestamp_high = -1;
    s_QpIp.u4_timestamp_low = -1;

    s_QpIp.u4_size = sizeof(ive_ctl_set_qp_ip_t);
    s_QpOp.u4_size = sizeof(ive_ctl_set_qp_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &s_QpIp, &s_QpOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set QP!\n";
}

void EncHelper::setEncMode(std::string encModeKey) {
    ive_ctl_set_enc_mode_ip_t sEncModeIp{};
    ive_ctl_set_enc_mode_op_t sEncModeOp{};

    sEncModeIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sEncModeIp.e_sub_cmd = IVE_CMD_CTL_SET_ENC_MODE;
    sEncModeIp.e_enc_mode = kEncMode.at(encModeKey);
    ;

    sEncModeIp.u4_timestamp_high = -1;
    sEncModeIp.u4_timestamp_low = -1;

    sEncModeIp.u4_size = sizeof(ive_ctl_set_enc_mode_ip_t);
    sEncModeOp.u4_size = sizeof(ive_ctl_set_enc_mode_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sEncModeIp, &sEncModeOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set encode mode!\n";
}

void EncHelper::setVbvParams() {
    ive_ctl_set_vbv_params_ip_t sVbvIp{};
    ive_ctl_set_vbv_params_op_t sVbvOp{};

    sVbvIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sVbvIp.e_sub_cmd = IVE_CMD_CTL_SET_VBV_PARAMS;
    sVbvIp.u4_vbv_buf_size = 0;
    sVbvIp.u4_vbv_buffer_delay = 1000;

    sVbvIp.u4_timestamp_high = -1;
    sVbvIp.u4_timestamp_low = -1;

    sVbvIp.u4_size = sizeof(ive_ctl_set_vbv_params_ip_t);
    sVbvOp.u4_size = sizeof(ive_ctl_set_vbv_params_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sVbvIp, &sVbvOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set VBV params!\n";
}

void EncHelper::setAirParams() {
    ive_ctl_set_air_params_ip_t sAirIp{};
    ive_ctl_set_air_params_op_t sAirOp{};

    sAirIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sAirIp.e_sub_cmd = IVE_CMD_CTL_SET_AIR_PARAMS;
    sAirIp.e_air_mode = kAirMode.at(mAirModeKey);
    sAirIp.u4_air_refresh_period = mIntraRefresh;

    sAirIp.u4_timestamp_high = -1;
    sAirIp.u4_timestamp_low = -1;

    sAirIp.u4_size = sizeof(ive_ctl_set_air_params_ip_t);
    sAirOp.u4_size = sizeof(ive_ctl_set_air_params_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sAirIp, &sAirOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set AIR params!\n";
}

void EncHelper::setMeParams() {
    ive_ctl_set_me_params_ip_t sMeParamsIp{};
    ive_ctl_set_me_params_op_t sMeParamsOp{};

    sMeParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sMeParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_ME_PARAMS;
    sMeParamsIp.u4_enable_fast_sad = mEnableFastSad;
    sMeParamsIp.u4_enable_alt_ref = mEnableAltRef;

    sMeParamsIp.u4_enable_hpel = mHalfPelEnable;
    sMeParamsIp.u4_enable_qpel = mQPelEnable;
    sMeParamsIp.u4_me_speed_preset = mMeSpeedPreset;
    sMeParamsIp.u4_srch_rng_x = mSearchRangeX;
    sMeParamsIp.u4_srch_rng_y = mSearchRangeY;

    sMeParamsIp.u4_timestamp_high = -1;
    sMeParamsIp.u4_timestamp_low = -1;

    sMeParamsIp.u4_size = sizeof(ive_ctl_set_me_params_ip_t);
    sMeParamsOp.u4_size = sizeof(ive_ctl_set_me_params_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sMeParamsIp, &sMeParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set ME params!\n";
}

void EncHelper::setGopParams() {
    ive_ctl_set_gop_params_ip_t sGopParamsIp{};
    ive_ctl_set_gop_params_op_t sGopParamsOp{};

    sGopParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sGopParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_GOP_PARAMS;

    sGopParamsIp.u4_i_frm_interval = mIInterval;
    sGopParamsIp.u4_idr_frm_interval = mIDRInterval;

    sGopParamsIp.u4_timestamp_high = -1;
    sGopParamsIp.u4_timestamp_low = -1;

    sGopParamsIp.u4_size = sizeof(ive_ctl_set_gop_params_ip_t);
    sGopParamsOp.u4_size = sizeof(ive_ctl_set_gop_params_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sGopParamsIp, &sGopParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set GOP params!\n";
}

void EncHelper::setProfileParams() {
    ive_ctl_set_profile_params_ip_t sProfileParamsIp{};
    ive_ctl_set_profile_params_op_t sProfileParamsOp{};

    sProfileParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sProfileParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_PROFILE_PARAMS;

    sProfileParamsIp.e_profile = kProfile.at(mProfileKey);
    if (sProfileParamsIp.e_profile == IV_PROFILE_BASE) {
        sProfileParamsIp.u4_entropy_coding_mode = 0;
    } else {
        sProfileParamsIp.u4_entropy_coding_mode = 1;
    }
    sProfileParamsIp.u4_timestamp_high = -1;
    sProfileParamsIp.u4_timestamp_low = -1;

    sProfileParamsIp.u4_size = sizeof(ive_ctl_set_profile_params_ip_t);
    sProfileParamsOp.u4_size = sizeof(ive_ctl_set_profile_params_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sProfileParamsIp, &sProfileParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set profile params!\n";
}

void EncHelper::setDeblockParams() {
    ive_ctl_set_deblock_params_ip_t sDeblockParamsIp{};
    ive_ctl_set_deblock_params_op_t sDeblockParamsOp{};

    sDeblockParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sDeblockParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_DEBLOCK_PARAMS;

    sDeblockParamsIp.u4_disable_deblock_level = mDisableDeblockLevel;

    sDeblockParamsIp.u4_timestamp_high = -1;
    sDeblockParamsIp.u4_timestamp_low = -1;

    sDeblockParamsIp.u4_size = sizeof(ive_ctl_set_deblock_params_ip_t);
    sDeblockParamsOp.u4_size = sizeof(ive_ctl_set_deblock_params_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sDeblockParamsIp, &sDeblockParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set deblock params!\n";
}

void EncHelper::setVuiParams() {
    ih264e_vui_ip_t sVuiParamsIp{};
    ih264e_vui_op_t sVuiParamsOp{};

    sVuiParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sVuiParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_VUI_PARAMS;

    sVuiParamsIp.u1_aspect_ratio_info_present_flag = mAspectRatioFlag;
    sVuiParamsIp.u1_video_signal_type_present_flag = 1;
    sVuiParamsIp.u1_colour_description_present_flag = 1;
    sVuiParamsIp.u1_nal_hrd_parameters_present_flag = mNalHrdFlag;
    sVuiParamsIp.u1_vcl_hrd_parameters_present_flag = mVclHrdFlag;

    sVuiParamsIp.u4_size = sizeof(ih264e_vui_ip_t);
    sVuiParamsOp.u4_size = sizeof(ih264e_vui_op_t);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, &sVuiParamsIp, &sVuiParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set VUI params!\n";
}

void EncHelper::setSeiMdcvParams() {
    ih264e_ctl_set_sei_mdcv_params_ip_t sSeiMdcvParamsIp{};
    ih264e_ctl_set_sei_mdcv_params_op_t sSeiMdcvParamsOp{};

    sSeiMdcvParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sSeiMdcvParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_SEI_MDCV_PARAMS;
    sSeiMdcvParamsIp.u1_sei_mdcv_params_present_flag = mSeiMdcvFlag;
    if (mSeiMdcvFlag) {
        for (int i4_count = 0; i4_count < NUM_SEI_MDCV_PRIMARIES; ++i4_count) {
            sSeiMdcvParamsIp.au2_display_primaries_x[i4_count] = 30000;
            sSeiMdcvParamsIp.au2_display_primaries_y[i4_count] = 35000;
        }
        sSeiMdcvParamsIp.u2_white_point_x = 30000;
        sSeiMdcvParamsIp.u2_white_point_y = 35000;
        sSeiMdcvParamsIp.u4_max_display_mastering_luminance = 100000000;
        sSeiMdcvParamsIp.u4_min_display_mastering_luminance = 50000;
    }

    sSeiMdcvParamsIp.u4_timestamp_high = -1;
    sSeiMdcvParamsIp.u4_timestamp_low = -1;

    sSeiMdcvParamsIp.u4_size = sizeof(ih264e_ctl_set_sei_mdcv_params_ip_t);
    sSeiMdcvParamsOp.u4_size = sizeof(ih264e_ctl_set_sei_mdcv_params_op_t);
    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sSeiMdcvParamsIp, &sSeiMdcvParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set SEI MDCV params!\n";
}

void EncHelper::setSeiCllParams() {
    ih264e_ctl_set_sei_cll_params_ip_t sSeiCllParamsIp{};
    ih264e_ctl_set_sei_cll_params_op_t sSeiCllParamsOp{};

    sSeiCllParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sSeiCllParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_SEI_CLL_PARAMS;
    sSeiCllParamsIp.u1_sei_cll_params_present_flag = mSeiCllFlag;
    if (mSeiCllFlag) {
        sSeiCllParamsIp.u2_max_content_light_level = 0;
        sSeiCllParamsIp.u2_max_pic_average_light_level = 0;
    }

    sSeiCllParamsIp.u4_timestamp_high = -1;
    sSeiCllParamsIp.u4_timestamp_low = -1;

    sSeiCllParamsIp.u4_size = sizeof(ih264e_ctl_set_sei_cll_params_ip_t);
    sSeiCllParamsOp.u4_size = sizeof(ih264e_ctl_set_sei_cll_params_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sSeiCllParamsIp, &sSeiCllParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set SEI CLL params!\n";
}

void EncHelper::setSeiAveParams() {
    ih264e_ctl_set_sei_ave_params_ip_t sSeiAveParamsIp{};
    ih264e_ctl_set_sei_ave_params_op_t sSeiAveParamsOp{};

    sSeiAveParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sSeiAveParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_SEI_AVE_PARAMS;
    sSeiAveParamsIp.u1_sei_ave_params_present_flag = mSeiAveFlag;
    if (mSeiAveFlag) {
        sSeiAveParamsIp.u4_ambient_illuminance = 1;
        sSeiAveParamsIp.u2_ambient_light_x = 0;
        sSeiAveParamsIp.u2_ambient_light_y = 0;
    }

    sSeiAveParamsIp.u4_timestamp_high = -1;
    sSeiAveParamsIp.u4_timestamp_low = -1;

    sSeiAveParamsIp.u4_size = sizeof(ih264e_ctl_set_sei_ave_params_ip_t);
    sSeiAveParamsOp.u4_size = sizeof(ih264e_ctl_set_sei_ave_params_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sSeiAveParamsIp, &sSeiAveParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set SEI AVE params!\n";
}

void EncHelper::setSeiCcvParams() {
    ih264e_ctl_set_sei_ccv_params_ip_t sSeiCcvParamsIp{};
    ih264e_ctl_set_sei_ccv_params_op_t sSeiCcvParamsOp{};

    sSeiCcvParamsIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sSeiCcvParamsIp.e_sub_cmd = IVE_CMD_CTL_SET_SEI_CCV_PARAMS;
    sSeiCcvParamsIp.u1_sei_ccv_params_present_flag = mSeiCcvFlag;
    if (mSeiCcvFlag) {
        sSeiCcvParamsIp.u1_ccv_cancel_flag = 0;
        sSeiCcvParamsIp.u1_ccv_persistence_flag = 1;
        sSeiCcvParamsIp.u1_ccv_primaries_present_flag = 1;
        sSeiCcvParamsIp.u1_ccv_min_luminance_value_present_flag = 1;
        sSeiCcvParamsIp.u1_ccv_max_luminance_value_present_flag = 1;
        sSeiCcvParamsIp.u1_ccv_avg_luminance_value_present_flag = 1;
        sSeiCcvParamsIp.u1_ccv_reserved_zero_2bits = 0;
        for (int i4_count = 0; i4_count < NUM_SEI_CCV_PRIMARIES; ++i4_count) {
            sSeiCcvParamsIp.ai4_ccv_primaries_x[i4_count] = 1;
            sSeiCcvParamsIp.ai4_ccv_primaries_y[i4_count] = 1;
        }
        sSeiCcvParamsIp.u4_ccv_min_luminance_value = 1;
        sSeiCcvParamsIp.u4_ccv_max_luminance_value = 1;
        sSeiCcvParamsIp.u4_ccv_avg_luminance_value = 1;
    }

    sSeiCcvParamsIp.u4_timestamp_high = -1;
    sSeiCcvParamsIp.u4_timestamp_low = -1;

    sSeiCcvParamsIp.u4_size = sizeof(ih264e_ctl_set_sei_ccv_params_ip_t);
    sSeiCcvParamsOp.u4_size = sizeof(ih264e_ctl_set_sei_ccv_params_op_t);

    IV_STATUS_T status =
        ive_api_function((iv_obj_t *)mCodecCtx, &sSeiCcvParamsIp, &sSeiCcvParamsOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set SEI CCV params!\n";
}

void EncHelper::logVersion() {
    ive_ctl_getversioninfo_ip_t sCtlIp{};
    ive_ctl_getversioninfo_op_t sCtlOp{};
    UWORD8 au1Buf[512];

    sCtlIp.e_cmd = IVE_CMD_VIDEO_CTL;
    sCtlIp.e_sub_cmd = IVE_CMD_CTL_GETVERSION;

    sCtlIp.u4_size = sizeof(ive_ctl_getversioninfo_ip_t);
    sCtlOp.u4_size = sizeof(ive_ctl_getversioninfo_op_t);
    sCtlIp.pu1_version = au1Buf;
    sCtlIp.u4_version_bufsize = sizeof(au1Buf);

    IV_STATUS_T status = ive_api_function((iv_obj_t *)mCodecCtx, (void *)&sCtlIp, (void *)&sCtlOp);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to get encoder version!\n";
}
