cmake_minimum_required(VERSION 3.13)
project(bf_jit)
set(CMAKE_CXX_STANDARD 17)

include_directories(libs/cxxopts/include)
add_subdirectory(src)