/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ih264_defs.h"
#include "ih264_typedefs.h"
#include "ih264e.h"
#include "ih264e_error.h"

#include "EncHelper.h"
#include "TestArgs.h"
#include "YuvSource.h"

const YuvSource kBbbCif("bbb_352x288_420p_30fps_32frames.yuv", 352, 288, 30, YUV_420P);
const YuvSource kFootballQvga("bbb_352x288_420p_30fps_32frames.yuv", 352, 288, 30, YUV_420P);

class EncTest : public ::testing::TestWithParam<std::tuple<YuvSource, int32_t>> {
  private:
    EncHelper mEnc;

  public:
    EncTest() {}
    ~EncTest() {}

    void process() { mEnc.process(); }

    void SetUp() override {
        std::tuple<YuvSource, int32_t /* bitRate */> params = GetParam();
        YuvSource yuvSource(std::get<0>(params));
        mEnc.mBitrate = std::get<1>(params) * 1024;
        mEnc.mInputFileName = gArgs->getPath() + yuvSource.mFileName;
        mEnc.mOutputFileName = gArgs->getPath() + "out.bin";
        mEnc.mWidth = yuvSource.mWidth;
        mEnc.mHeight = yuvSource.mHeight;
        mEnc.mTargetFrameRate = mEnc.mSourceFrameRate = yuvSource.mFrameRate;
        mEnc.mSaveOutputFile = true;

        mEnc.create();
    }

    void TearDown() override { mEnc.destroy(); }
};

TEST_P(EncTest, EncodeTest) { ASSERT_NO_FATAL_FAILURE(process()) << "Failed to Encode"; }

INSTANTIATE_TEST_SUITE_P(EncodeTest, EncTest,
                         ::testing::Values(std::make_tuple(kBbbCif, 2048),
                                           std::make_tuple(kFootballQvga, 1024)));
