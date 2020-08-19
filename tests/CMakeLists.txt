cmake_minimum_required(VERSION 3.10)

include(GoogleTest)
include_directories(${CMAKE_SOURCE_DIR}/include)

set(TARGET_NAME ${PROJECT_NAME}_test)

add_executable(${TARGET_NAME}
                main.cpp
                tests.cpp
                )

target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads gtest gmock)

gtest_discover_tests(${TARGET_NAME} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})