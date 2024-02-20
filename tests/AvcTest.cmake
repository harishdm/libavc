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
  AVCTEST_SRCS
  "${AVC_ROOT}/tests/TestMain.cpp"
  "${AVC_ROOT}/tests/EncTest.cpp"
  "${AVC_ROOT}/tests/DecTest.cpp"
  "${AVC_ROOT}/tests/EncHelper.cpp"
  "${AVC_ROOT}/tests/DecHelper.cpp"
  "${AVC_ROOT}/tests/Md5Wrapper.cpp")

libavc_add_executable(AvcTest libavcenc libavcdec
    SOURCES ${AVCTEST_SRCS}
    INCLUDES "${AVC_ROOT}/third_party/googletest/googletest/include")
target_link_libraries(AvcTest
    libavcenc
    libavcdec
    crypto # for md5
    ${AVC_ROOT}/third_party/build/googletest/src/googletest-build/lib/libgtest.a
    ${AVC_ROOT}/third_party/build/googletest/src/googletest-build/lib/libgtest_main.a)

add_dependencies(AvcTest googletest)
