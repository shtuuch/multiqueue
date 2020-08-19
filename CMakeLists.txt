cmake_minimum_required(VERSION 3.10)
project(multi_queue)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_subdirectory(src)

option (BUILD_TESTS "enable tests" ON)

if (${BUILD_TESTS})

    set(INSTALL_GTEST OFF CACHE BOOL "")
    set(gtest_force_shared_crt ON CACHE BOOL "")
    add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/googletest)
    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
    include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

    enable_testing()
    add_subdirectory(tests)

endif()
