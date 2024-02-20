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

#pragma once
#include "TestCommon.h"
#include <cstddef>
#include <string>

class EncHelper {
  public:
    EncHelper();
    ~EncHelper();

    // Methods to initialize arguments
    void initArgsDefault();
    void initArgsRandom();
    void initArgsFuzzer();

    void create();
    void destroy();
    void process();

    size_t mWidth;
    size_t mHeight;
    size_t mSourceFrameRate;
    size_t mTargetFrameRate;
    size_t mBitrate;
    size_t mNumBFrames;

    size_t mMaxWidth;
    size_t mMaxHeight;
    size_t mMaxBitrate;
    size_t mMaxFrameRate;

    ColorFormat mInputYuvFormatKey;
    ColorFormat mReconYuvFormatKey;
    std::string mRcModeKey;
    std::string mAirModeKey;
    std::string mSpeedKey;
    std::string mProfileKey;

    size_t mMaxLevel;

    std::string mInputFileName;
    std::string mOutputFileName;
    std::string mReconFileName;

    bool mSaveOutputFile;
    bool mSaveReconFile;

    size_t mInitQpI;
    size_t mInitQpP;
    size_t mInitQpB;

    size_t mMaxQpI;
    size_t mMaxQpP;
    size_t mMaxQpB;

    size_t mMinQpI;
    size_t mMinQpP;
    size_t mMinQpB;

    size_t mNumFrames;

    size_t mNumThreads;

    bool mIntra4x4;
    bool mConstrainedIntraFlag;
    bool mHalfPelEnable;
    bool mQPelEnable;
    size_t mIntraRefresh;
    size_t mSliceParam;
    bool mEnableFastSad;
    bool mEnableAltRef;
    bool mSeiCllFlag;
    bool mSeiAveFlag;
    bool mSeiCcvFlag;
    bool mSeiMdcvFlag;
    bool mAspectRatioFlag;
    bool mNalHrdFlag;
    bool mVclHrdFlag;
    size_t mMeSpeedPreset;
    size_t mSearchRangeX;
    size_t mSearchRangeY;
    size_t mIDRInterval;
    size_t mIInterval;
    size_t mDisableDeblockLevel;

  private:
    void setEncMode(std::string encModeKey);
    void setDimensions();
    void setNumCores();
    void setFrameRate();
    void setIpeParams();
    void setBitRate();
    void setFrameType(std::string frameTypeKey);
    void setQp();
    void setAirParams();
    void setMeParams();
    void setGopParams();
    void setProfileParams();
    void setDeblockParams();
    void setVbvParams();
    void setSeiMdcvParams();
    void setSeiCllParams();
    void setSeiAveParams();
    void setSeiCcvParams();
    void setDefault();
    void setVuiParams();
    void getBufInfo();
    void logVersion();

    void *mCodecCtx = nullptr;
    void *mMemRecordsBuffer = nullptr;
    size_t mNumMemRecords = 0;

    FILE *mInputFp = nullptr;
    FILE *mOutputFp = nullptr;
};
