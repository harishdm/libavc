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
#include <cstdio>
#include <string>
#include <vector>

#include "Md5Wrapper.h"
#include "TestCommon.h"

constexpr size_t kMaxComponents = 3;

// Class to handle YUV buffers
class RawBuf {
  public:
    RawBuf(size_t width, size_t height, size_t stride, ColorFormat format) {
        switch (format) {
        case YUV_420SP_UV:
            [[fallthrough]];
        case YUV_420SP_VU:
            mWidth[0] = width;
            mWidth[1] = width;

            mHeight[0] = height;
            mHeight[1] = height / 2;

            mStride[0] = stride;
            mStride[1] = stride;
            mNumBufs = 2;
            break;
        case YUV_420P:
            [[fallthrough]];
        default:
            mWidth[0] = width;
            mWidth[1] = width / 2;
            mWidth[2] = width / 2;

            mHeight[0] = height;
            mHeight[1] = height / 2;
            mHeight[2] = height / 2;

            mStride[0] = stride;
            mStride[1] = stride / 2;
            mStride[2] = stride / 2;
            mNumBufs = 3;
            break;
        }

        mFormat = format;
    }

    bool alloc() {
        for (size_t i = 0; i < kMaxComponents; i++) {
            mBuffer[i].resize(mStride[i] * mHeight[i]);
        }
        return true;
    }

    bool read(FILE *fp) {
        size_t bytes;
        size_t i;
        size_t comp;
        size_t num_comp;

        for (comp = 0; comp < kMaxComponents; comp++) {
            size_t width = mWidth[comp];
            size_t height = mHeight[comp];
            size_t stride = mStride[comp];
            uint8_t *buf = mBuffer[comp].data();

            for (i = 0; i < height; i++) {
                bytes = fread(buf, sizeof(uint8_t), width, fp);
                if (bytes != width) {
                    return false;
                }
                buf += stride;
            }
        }

        return true;
    }

    bool write(FILE *fp) {
        size_t bytes;
        size_t i;
        size_t comp;
        size_t num_comp;

        for (comp = 0; comp < kMaxComponents; comp++) {
            size_t width = mWidth[comp];
            size_t height = mHeight[comp];
            size_t stride = mStride[comp];
            uint8_t *buf = mBuffer[comp].data();

            for (i = 0; i < height; i++) {
                bytes = fwrite(buf, sizeof(uint8_t), width, fp);
                if (bytes != width) {
                    return false;
                }
                buf += stride;
            }
        }

        return true;
    }

    void computeMd5(unsigned char md5sum[kMaxComponents][kMD5Length]) {
        size_t bytes;
        size_t i;
        size_t comp;
        size_t num_comp;

        for (comp = 0; comp < kMaxComponents; comp++) {
            Md5Wrapper md5;
            md5.init();
            size_t width = mWidth[comp];
            size_t height = mHeight[comp];
            size_t stride = mStride[comp];
            uint8_t *buf = mBuffer[comp].data();

            for (i = 0; i < height; i++) {
                md5.update(buf, width);
                buf += stride;
            }
            md5.result(md5sum[comp]);
        }
    }

    ColorFormat mFormat;
    std::vector<uint8_t> mBuffer[kMaxComponents]{};
    size_t mWidth[kMaxComponents]{};
    size_t mHeight[kMaxComponents]{};
    size_t mStride[kMaxComponents]{};
    size_t mNumBufs;
};
