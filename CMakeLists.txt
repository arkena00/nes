cmake_minimum_required(VERSION 3.12)
project(nes)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")


add_executable(nes_pipeline "source/pipeline.cpp")
add_executable(nes_thread_pool "source/thread_pool.cpp")