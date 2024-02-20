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

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

#include "BitsBuf.h"
#include "RawBuf.h"
#include "TestCommon.h"
#include "ih264_typedefs.h"
#include "ih264d.h"
#include "iv.h"
#include "ivd.h"

#include "DecHelper.h"
#include "Md5Wrapper.h"

#define ivd_api_function ih264d_api_function

constexpr IVD_ARCH_T kArch[] = {ARCH_ARM_NONEON,    ARCH_ARM_A9Q,     ARCH_ARM_NEONINTR,
                                ARCH_ARMV8_GENERIC, ARCH_X86_GENERIC, ARCH_X86_SSSE3,
                                ARCH_X86_SSE42};

const std::map<ColorFormat, IV_COLOR_FORMAT_T> kColorFormat{
    {YUV_420P, IV_YUV_420P}, {YUV_420SP_UV, IV_YUV_420SP_UV}, {YUV_420SP_VU, IV_YUV_420SP_VU}};

const std::map<std::string, IVD_VIDEO_DECODE_MODE_T> kDecMode{{"Picture", IVD_DECODE_FRAME},
                                                              {"Header", IVD_DECODE_HEADER}};
void *iv_aligned_malloc(void *ctxt, WORD32 alignment, WORD32 size) {
    void *buf = nullptr;
    (void)ctxt;
    if (0 != posix_memalign(&buf, alignment, size)) {
        return nullptr;
    }
    return buf;
}

void iv_aligned_free(void *ctxt, void *buf) {
    (void)ctxt;
    free(buf);
}

DecHelper::DecHelper() { initArgsDefault(); }

DecHelper::~DecHelper() {}

void DecHelper::initArgsDefault() {
    mYuvFormatKey = YUV_420P;
    mInputFileName = "";
    mOutputFileName = "";
    mSaveOutputFile = false;
    mNumFrames = std::numeric_limits<size_t>::max();
    mNumThreads = 1;
    mArch = ARCH_NA;
}

void DecHelper::initArgsRandom() {}

#ifdef ENABLE_FUZZER
void DecHelper::initArgsFuzzer(FuzzedDataProvider *fdp) {
    mFdp = fdp;
    mYuvFormatKey = YUV_420P;
    mInputFileName = "";
    mOutputFileName = "";
    mSaveOutputFile = false;
    mNumFrames = 100;
    mNumThreads = fdp->ConsumeIntegralInRange(1, 4);
    mArch = fdp->PickValueInArray(kArch);
}
#endif

void DecHelper::create() {
    mInputFp = fopen(mInputFileName.c_str(), "rb");
    ASSERT_NE(mInputFp, nullptr) << "Failed to open input file: " << mInputFileName;

    // MD5 file is optional, so mRefMd5Fp may be nullptr after the following
    mRefMd5Fp = fopen(mRefMd5FileName.c_str(), "rb");

    mOutputFp = fopen(mOutputFileName.c_str(), "wb");
    ASSERT_NE(mOutputFp, nullptr) << "Failed to open output file: " << mOutputFileName;

    IV_API_CALL_STATUS_T status;
    ih264d_create_ip_t create_ip{};
    ih264d_create_op_t create_op{};
    void *fxns = (void *)&ivd_api_function;

    create_ip.s_ivd_create_ip_t.e_cmd = IVD_CMD_CREATE;
    create_ip.s_ivd_create_ip_t.u4_share_disp_buf = 0;
    create_ip.s_ivd_create_ip_t.e_output_format = kColorFormat.at(mYuvFormatKey);
    create_ip.s_ivd_create_ip_t.pf_aligned_alloc = iv_aligned_malloc;
    create_ip.s_ivd_create_ip_t.pf_aligned_free = iv_aligned_free;
    create_ip.u4_keep_threads_active = 1;
    create_ip.s_ivd_create_ip_t.pv_mem_ctxt = nullptr;
    create_ip.s_ivd_create_ip_t.u4_size = sizeof(ih264d_create_ip_t);
    create_op.s_ivd_create_op_t.u4_size = sizeof(ih264d_create_op_t);

    status = ivd_api_function(nullptr, (void *)&create_ip, (void *)&create_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to create codec.";

    iv_obj_t *codecCtx = (iv_obj_t *)create_op.s_ivd_create_op_t.pv_handle;
    codecCtx->pv_fxns = fxns;
    codecCtx->u4_size = sizeof(iv_obj_t);

    mCodecCtx = (void *)codecCtx;

    ASSERT_NO_FATAL_FAILURE(setCores());

    ASSERT_NO_FATAL_FAILURE(setArchitecture());
}

void DecHelper::destroy() {
    IV_API_CALL_STATUS_T status;
    ivd_delete_ip_t delete_ip{};
    ivd_delete_op_t delete_op{};

    delete_ip.e_cmd = IVD_CMD_DELETE;
    delete_ip.u4_size = sizeof(ivd_delete_ip_t);
    delete_op.u4_size = sizeof(ivd_delete_op_t);

    status = ivd_api_function((iv_obj_t *)mCodecCtx, (void *)&delete_ip, (void *)&delete_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to delete codec.";

    if (mInputFp)
        fclose(mInputFp);
    if (mRefMd5Fp)
        fclose(mRefMd5Fp);
    if (mOutputFp)
        fclose(mOutputFp);
    mCodecCtx = nullptr;
}

void DecHelper::resetCodec() {
    IV_API_CALL_STATUS_T status;
    ivd_ctl_reset_ip_t s_ctl_ip{};
    ivd_ctl_reset_op_t s_ctl_op{};

    s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    s_ctl_ip.e_sub_cmd = IVD_CMD_CTL_RESET;
    s_ctl_ip.u4_size = sizeof(ivd_ctl_reset_ip_t);
    s_ctl_op.u4_size = sizeof(ivd_ctl_reset_op_t);

    status = ivd_api_function((iv_obj_t *)mCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to delete codec.";
}

void DecHelper::setCores() {
    IV_API_CALL_STATUS_T status;
    ih264d_ctl_set_num_cores_ip_t s_ctl_ip{};
    ih264d_ctl_set_num_cores_op_t s_ctl_op{};

    s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    s_ctl_ip.e_sub_cmd = (IVD_CONTROL_API_COMMAND_TYPE_T)IH264D_CMD_CTL_SET_NUM_CORES;
    s_ctl_ip.u4_num_cores = mNumThreads;
    s_ctl_ip.u4_size = sizeof(ih264d_ctl_set_num_cores_ip_t);
    s_ctl_op.u4_size = sizeof(ih264d_ctl_set_num_cores_op_t);

    status = ivd_api_function((iv_obj_t *)mCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to number of threads.";
}

void DecHelper::setParams(std::string decModeKey) {
    IV_API_CALL_STATUS_T status;
    ivd_ctl_set_config_ip_t s_ctl_ip{};
    ivd_ctl_set_config_op_t s_ctl_op{};

    s_ctl_ip.u4_disp_wd = 0;
    s_ctl_ip.e_frm_skip_mode = IVD_SKIP_NONE;
    s_ctl_ip.e_frm_out_mode = IVD_DISPLAY_FRAME_OUT;
    s_ctl_ip.e_vid_dec_mode = kDecMode.at(decModeKey);
    s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    s_ctl_ip.e_sub_cmd = IVD_CMD_CTL_SETPARAMS;
    s_ctl_ip.u4_size = sizeof(ivd_ctl_set_config_ip_t);
    s_ctl_op.u4_size = sizeof(ivd_ctl_set_config_op_t);

    status = ivd_api_function((iv_obj_t *)mCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to parameters.";
}

void DecHelper::setArchitecture() {
    IV_API_CALL_STATUS_T status;
    ih264d_ctl_set_processor_ip_t s_ctl_ip{};
    ih264d_ctl_set_processor_op_t s_ctl_op{};

    s_ctl_ip.e_cmd = IVD_CMD_VIDEO_CTL;
    s_ctl_ip.e_sub_cmd = (IVD_CONTROL_API_COMMAND_TYPE_T)IH264D_CMD_CTL_SET_PROCESSOR;
    s_ctl_ip.u4_arch = mArch;
    s_ctl_ip.u4_soc = SOC_GENERIC;
    s_ctl_ip.u4_size = sizeof(ih264d_ctl_set_processor_ip_t);
    s_ctl_op.u4_size = sizeof(ih264d_ctl_set_processor_op_t);

    status = ivd_api_function((iv_obj_t *)mCodecCtx, (void *)&s_ctl_ip, (void *)&s_ctl_op);
    ASSERT_EQ(status, IV_SUCCESS) << "Failed to set architecture.";
}

bool DecHelper::verifyMd5(unsigned char tstMd5[kMaxComponents][kMD5Length]) {
    if (nullptr == mRefMd5Fp)
        return true;
    size_t bytes;
    size_t i;
    size_t comp;
    size_t num_comp;
    unsigned char refMd5[kMaxComponents][kMD5Length];

    bytes = fread(refMd5, sizeof(uint8_t), kMaxComponents * kMD5Length, mRefMd5Fp);
    if (bytes != kMaxComponents * kMD5Length) {
        return false;
    }

    return memcmp(tstMd5, refMd5, kMaxComponents * kMD5Length) ? false : true;
}

void DecHelper::process() {
    iv_obj_t *codecCtx = (iv_obj_t *)mCodecCtx;
    IV_API_CALL_STATUS_T status;
    ASSERT_NO_FATAL_FAILURE(setParams("Header"));
    size_t width = 0, height = 0;
    size_t numHeaderDecodeCalls = mNumFrames;
    BitsBuf headerBuf(256 * 1024);
    headerBuf.alloc();
    headerBuf.read(mInputFp, 256 * 1024);
    uint8_t *data = headerBuf.mBuffer.data();
    size_t size = headerBuf.mBufferSize;
    size_t filePos = 0;

    do {
        IV_API_CALL_STATUS_T ret;
        ivd_video_decode_ip_t dec_ip{};
        ivd_video_decode_op_t dec_op{};
        size_t bytes_consumed;

        dec_ip.e_cmd = IVD_CMD_VIDEO_DECODE;
        dec_ip.u4_ts = 0;
        dec_ip.pv_stream_buffer = (void *)data;
        dec_ip.u4_num_Bytes = size;
        dec_ip.u4_size = sizeof(ivd_video_decode_ip_t);
        dec_op.u4_size = sizeof(ivd_video_decode_op_t);

        status = ivd_api_function(codecCtx, (void *)&dec_ip, (void *)&dec_op);
        ASSERT_EQ(status, IV_SUCCESS) << "Failed to decode header";
        bytes_consumed = dec_op.u4_num_bytes_consumed;
        filePos += bytes_consumed;
        data += bytes_consumed;
        size -= bytes_consumed;
        if (width == 0 || height == 0) {
            width = dec_op.u4_pic_wd;
            height = dec_op.u4_pic_ht;
        }

        /* Break after successful header decode */
        if (width && height) {
            break;
        }
    } while (size > 0 && numHeaderDecodeCalls--);

    ASSERT_NO_FATAL_FAILURE(setParams("Picture"));

    uint32_t numFrame = 0;
    size_t numFramesToDecode = mNumFrames;

    RawBuf outputBuf(width, height, width, mYuvFormatKey);
    outputBuf.alloc();
    ivd_out_bufdesc_t sOutBuf;
    for (int i = 0; i < kMaxComponents; i++) {
        sOutBuf.u4_min_out_buf_size[i] = outputBuf.mStride[i] * outputBuf.mHeight[i];
        sOutBuf.pu1_bufs[i] = (UWORD8 *)outputBuf.mBuffer[i].data();
    }
    sOutBuf.u4_num_bufs = outputBuf.mNumBufs;

    BitsBuf inputBuf(width * height * 3 / 2);
    inputBuf.alloc();

    while (numFramesToDecode > 0) {
#ifndef ENABLE_FUZZER
        fseek(mInputFp, filePos, SEEK_SET);
        if (!inputBuf.read(mInputFp, width * height * 3 / 2)) {
            break;
        }
#else
        if (!inputBuf.read(mFdp, width * height * 3 / 2)) {
            break;
        }
#endif
        data = inputBuf.mBuffer.data();
        size = inputBuf.mBufferSize;

        IV_API_CALL_STATUS_T ret;
        ivd_video_decode_ip_t dec_ip{};
        ivd_video_decode_op_t dec_op{};

        dec_ip.e_cmd = IVD_CMD_VIDEO_DECODE;
        dec_ip.u4_ts = 0;
        dec_ip.pv_stream_buffer = (void *)data;
        dec_ip.u4_num_Bytes = size;
        dec_ip.u4_size = sizeof(ivd_video_decode_ip_t);
        dec_ip.s_out_buffer = sOutBuf;

        dec_op.u4_size = sizeof(ivd_video_decode_op_t);

        ret = ivd_api_function(codecCtx, (void *)&dec_ip, (void *)&dec_op);
        ASSERT_EQ(status, IV_SUCCESS) << "Failed to decode frame";

        /* In case of change in resolution, reset codec and feed the same data again
         */
        if (IVD_RES_CHANGED == (dec_op.u4_error_code & 0xFF)) {
            resetCodec();
            ret = ivd_api_function(codecCtx, (void *)&dec_ip, (void *)&dec_op);
            ASSERT_EQ(status, IV_SUCCESS) << "Failed to reset decoder";
        }
        filePos += dec_op.u4_num_bytes_consumed;

        if (dec_op.u4_output_present) {
            if (mOutputFp) {
                ASSERT_NE(outputBuf.write(mOutputFp), false)
                    << "Failed to write the output!" << mOutputFileName;
            }
            unsigned char tstMd5[kMaxComponents][kMD5Length];
            outputBuf.computeMd5(tstMd5);
            ASSERT_NE(verifyMd5(tstMd5), false) << "MD5 mismatch!";
        }
        numFramesToDecode--;
        numFrame++;
    }
}
