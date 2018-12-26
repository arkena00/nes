cmake_minimum_required(VERSION 3.12)
project(nes)

set(CMAKE_CXX_STANDARD 17)

include_directories("include")


add_executable(nes_pipeline "source/pipeline.cpp")
add_executable(nes_thread_pool "source/thread_pool.cpp")

message(status ${CMAKE_CURRENT_SOURCE_DIR}/third_party/benchmark/lib/benchmark)

link_directories(third_party/benchmark/lib)

# bench
add_executable(bench_pipeline "source/bench.cpp")
target_include_directories(bench_pipeline PUBLIC include third_party/benchmark/include)
target_link_libraries(bench_pipeline PUBLIC benchmark benchmark_main shlwapi)
set_target_properties(bench_pipeline PROPERTIES RUNTIME_OUTPUT_DIRECTORY "bin/bench")