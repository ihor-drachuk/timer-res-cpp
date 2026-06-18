cmake_minimum_required(VERSION 3.21)
project(timer-res LANGUAGES CXX)

# Fully static CRT (/MT) so the binaries carry no vcruntime/ucrt DLL deps.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(libs/timer-res-core)
add_subdirectory(apps/timer-res-test-tool)
add_subdirectory(apps/timer-res)
