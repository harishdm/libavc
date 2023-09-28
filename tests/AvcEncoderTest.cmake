include(ExternalProject)
ExternalProject_Add(googletest
    GIT_REPOSITORY https://android.googlesource.com/platform/external/googletest
    GIT_TAG main
    PREFIX ${AVC_ROOT}/third_party/build/googletest
    SOURCE_DIR ${AVC_ROOT}/third_party/googletest
    TMP_DIR ${AVC_ROOT}/third_party/build/googletest/tmp
    INSTALL_COMMAND ""
)

list(
  APPEND
  AVCENCODERTEST_SRCS
  "${AVC_ROOT}/tests/AvcEncoderTest.cpp")

libavc_add_executable(AvcEncoderTest libavcenc SOURCES ${AVCENCODERTEST_SRCS})
target_link_libraries(AvcEncoderTest
    ${AVC_ROOT}/third_party/build/googletest/src/googletest-build/lib/libgtest.a
    ${AVC_ROOT}/third_party/build/googletest/src/googletest-build/lib/libgtest_main.a
)
