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

#include "Md5Wrapper.h"

#include <openssl/md5.h>

#define MD5WrapperCtxt MD5_CTX
#define MD5WrapperInit MD5_Init
#define MD5WrapperUpdate MD5_Update
#define MD5WrapperFinal MD5_Final

struct Md5Wrapper::Impl {
    MD5WrapperCtxt mCtxt;
};

Md5Wrapper::Md5Wrapper() : mImpl(new Impl()) {}
Md5Wrapper::~Md5Wrapper() {}

void Md5Wrapper::init() { MD5WrapperInit(&mImpl->mCtxt); }

void Md5Wrapper::update(uint8_t *buf, size_t size) { MD5WrapperUpdate(&mImpl->mCtxt, buf, size); }

void Md5Wrapper::result(uint8_t *output) { MD5WrapperFinal(output, &mImpl->mCtxt); }
