cmake_minimum_required(VERSION 3.12)
project(nes)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")

file(GLOB_RECURSE SOURCES "source/*.cpp" "include/*.hpp" "include/*.tpp")

add_executable(nes ${SOURCES})