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

// Class to handle bitstream buffers
class BitsBuf {
  public:
    BitsBuf(size_t size) { mBufferSize = size; }

    bool alloc() {
        mBuffer.resize(mBufferSize);
        return true;
    }

    bool read(FILE *fp, size_t numBytes) {
        size_t bytesRead = fread(mBuffer.data(), sizeof(uint8_t), numBytes, fp);
        return bytesRead != 0;
    }

    bool write(FILE *fp, size_t numBytes) {
        size_t bytesWritten = fwrite(mBuffer.data(), sizeof(uint8_t), numBytes, fp);
        return bytesWritten == numBytes;
    }

    std::vector<uint8_t> mBuffer{};
    size_t mBufferSize = 0;
};
