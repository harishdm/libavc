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
#include <cstddef>
#include <string>

#include "RawBuf.h"
#include "TestCommon.h"

#ifdef ENABLE_FUZZER
#include <fuzzer/FuzzedDataProvider.h>
#endif

class DecHelper {
  public:
    DecHelper();
    ~DecHelper();

    // Methods to initialize arguments
    void initArgsDefault();
    void initArgsRandom();
#ifdef ENABLE_FUZZER
    void initArgsFuzzer(FuzzedDataProvider *fdp);
#endif

    void create();
    void destroy();
    void resetCodec();
    void setCores();
    void setParams(std::string decModeKey);
    void setArchitecture();
    void process();

    ColorFormat mYuvFormatKey;
    std::string mInputFileName;
    std::string mOutputFileName;
    std::string mRefMd5FileName;
    bool mSaveOutputFile;
    size_t mNumFrames;
    size_t mNumThreads;
    size_t mArch;

  private:
    void setDecMode(std::string decModeKey);
    bool verifyMd5(unsigned char tstMd5[kMaxComponents][kMD5Length]);
    void *mCodecCtx = nullptr;
    FILE *mInputFp = nullptr;
    FILE *mOutputFp = nullptr;
    FILE *mRefMd5Fp = nullptr;
#ifdef ENABLE_FUZZER
    FuzzedDataProvider *mFdp;
#endif
};
