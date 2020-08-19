cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

set(TARGET_NAME ${PROJECT_NAME})

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(${TARGET_NAME} main.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE Threads::Threads)