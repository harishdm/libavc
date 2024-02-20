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
#include <cstdint>
#include <vector>

class YuvSource {
  public:
    YuvSource(std::string filename, size_t width, size_t height, size_t frameRate,
              ColorFormat format) {
        mFileName = filename;
        mWidth = width;
        mHeight = height;
        mFrameRate = frameRate;
        mFormat = format;
    }
    std::string mFileName;
    size_t mWidth;
    size_t mHeight;
    size_t mFrameRate;
    size_t mFormat;
};
