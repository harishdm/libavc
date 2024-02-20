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

#include "TestArgs.h"
#include "TestCommon.h"

TestArgs *gArgs = nullptr;

int32_t main(int argc, char **argv) {
    gArgs = new TestArgs();
    ::testing::AddGlobalTestEnvironment(gArgs);
    ::testing::InitGoogleTest(&argc, argv);
    uint8_t status = gArgs->initFromOptions(argc, argv);
    if (status == 0) {
        status = RUN_ALL_TESTS();
    }
    return status;
}
