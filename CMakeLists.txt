cmake_minimum_required(VERSION 3.28)
project(sentinel-siem VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(SENTINEL_BUILD_TESTS "Build unit tests" ON)
option(SENTINEL_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(SENTINEL_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)

# Sanitizers
if(SENTINEL_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

add_subdirectory(engine)
