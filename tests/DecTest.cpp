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

#include "DecHelper.h"
#include "BitsSource.h"
#include "TestArgs.h"

const BitsSource kBbbCif("football_qvga_bp.h264");
const BitsSource kFootballQvga("bbb_350x286_420p_30fps_32frames_hp.h264");

class DecTest : public ::testing::TestWithParam<std::tuple<BitsSource>> {
  private:
    DecHelper mDec;

  public:
    DecTest() {}
    ~DecTest() {}

    void process() { mDec.process(); }

    void SetUp() override {
        std::tuple<BitsSource> params = GetParam();
        BitsSource bitsSource(std::get<0>(params));

        mDec.mInputFileName = gArgs->getPath() + bitsSource.mFileName;
        mDec.mRefMd5FileName = gArgs->getPath() + bitsSource.mMd5FileName;
        mDec.mOutputFileName = gArgs->getPath() + "out.yuv";
        mDec.mSaveOutputFile = true;

        mDec.create();
    }

    void TearDown() override { mDec.destroy(); }
};

TEST_P(DecTest, DecodeTest) { ASSERT_NO_FATAL_FAILURE(process()) << "Failed to Decode"; }

INSTANTIATE_TEST_SUITE_P(DecodeTest, DecTest,
                         ::testing::Values(std::make_tuple(kBbbCif), std::make_tuple(kFootballQvga)));
