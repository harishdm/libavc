# AvcTest
The Avc Test Suite validates the Avc encoder and decoder.

## Linux x86/x64

###  Requirements
- cmake (3.9.1 or above)
- make
- clang (12.0 or above)

### Steps to build
Clone libavc repository
```
$ git clone https://android.googlesource.com/platform/external/libavc
```
Create a directory inside libavc and change directory
```
 $ cd libavc
 $ mkdir build
 $ cd build
```

Build with -DENABLE_TESTS=1.
```
 $ cmake .. -DENABLE_TESTS=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
   -DCMAKE_BUILD_TYPE=Debug
 $ make
```

Optionally, enable sanitizers by passing -DSANITIZE
```
 $ cmake .. -DENABLE_TESTS=1 -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
   -DCMAKE_BUILD_TYPE=Debug -DSANITIZE=fuzzer-no-link,address,\
   signed-integer-overflow,unsigned-integer-overflow
 $ make
```

The media files for the tests are present [at](https://dl.google.com/android-unittest/media/external/libavc/tests/AvcTestRes-1.0.zip).
Download and extract these the current folder.

usage: AvcTest -P \<path_to_the local folder\>

```
$./AvcTest -P ./
```

## Android

Run the following steps to build the test suite:
```
m AvcTest
```

To test 64-bit binary push binaries from nativetest64.
```
adb push ${OUT}/data/nativetest64/AvcTest/AvcTest /data/local/tmp/
```

To test 32-bit binary push binaries from nativetest.
```
adb push ${OUT}/data/nativetest/AvcTest/AvcTest /data/local/tmp/
```

The resource file for the tests is taken from [here](https://dl.google.com/android-unittest/media/external/libavc/tests/AvcTestRes-1.0.zip)

Download, unzip and push these files into device for testing.

```
adb push AvcTestRes-1.0 /sdcard/test/
```

usage: AvcTest -P \<path_to_folder\>
```
adb shell /data/local/tmp/AvcTest -P /sdcard/test/AvcTestRes-1.0/
```
Alternatively, the test can also be run using atest command.

```
atest AvcTest
```
