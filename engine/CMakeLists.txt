cmake_minimum_required(VERSION 3.28)

# ============================================================================
# Dependencies via FetchContent
# ============================================================================
include(FetchContent)

# spdlog - Fast logging
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1
)

# Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# nlohmann/json
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

# zstd compression
FetchContent_Declare(
  zstd
  GIT_REPOSITORY https://github.com/facebook/zstd.git
  GIT_TAG v1.5.6
  SOURCE_SUBDIR build/cmake
)

# lz4 compression
FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4.git
  GIT_TAG v1.10.0
  SOURCE_SUBDIR build/cmake
)

FetchContent_MakeAvailable(spdlog nlohmann_json)

# zstd: build only the static library
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zstd)

# lz4
set(LZ4_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lz4)

# ============================================================================
# gRPC and Protobuf
# ============================================================================
find_package(Protobuf QUIET)
find_package(gRPC QUIET)

if(Protobuf_FOUND AND gRPC_FOUND)
  message(STATUS "Found system gRPC and Protobuf")
else()
  message(STATUS "gRPC/Protobuf not found - gRPC services will be disabled")
  message(STATUS "Install gRPC: https://grpc.io/docs/languages/cpp/quickstart/")
  set(SENTINEL_GRPC_ENABLED OFF)
endif()

# ============================================================================
# Proto generation (if gRPC available)
# ============================================================================
if(NOT DEFINED SENTINEL_GRPC_ENABLED OR SENTINEL_GRPC_ENABLED)
  set(PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
  set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

  set(PROTO_FILES
    ${PROTO_DIR}/common.proto
    ${PROTO_DIR}/ingest.proto
    ${PROTO_DIR}/search.proto
    ${PROTO_DIR}/indexer.proto
  )
endif()

# ============================================================================
# Engine Core Library
# ============================================================================
add_library(sentinel_engine STATIC
  # Common
  src/common/config.cpp
  src/common/logging.cpp
  src/common/thread_pool.cpp
  src/common/metrics.cpp

  # Storage
  src/storage/block_store.cpp
  src/storage/mmap_reader.cpp
  src/storage/compression.cpp

  # Indexer
  src/indexer/segment.cpp
  src/indexer/inverted_index.cpp
  src/indexer/bloom_filter.cpp
  src/indexer/compaction.cpp
  src/indexer/wal.cpp

  # Search
  src/search/executor.cpp
  src/search/filter.cpp
  src/search/aggregator.cpp
  src/search/result_set.cpp

  # SPL
  src/spl/lexer.cpp
  src/spl/parser.cpp
  src/spl/ast.cpp
  src/spl/semantic.cpp
  src/spl/optimizer.cpp
  src/spl/codegen.cpp

  # Ingest
  src/ingest/pipeline.cpp
  src/ingest/syslog_receiver.cpp
  src/ingest/file_monitor.cpp
  src/ingest/line_breaker.cpp
  src/ingest/timestamp_extractor.cpp
)

target_include_directories(sentinel_engine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sentinel_engine PUBLIC
  spdlog::spdlog
  nlohmann_json::nlohmann_json
  libzstd_static
  lz4_static
)

# ============================================================================
# Engine Executable
# ============================================================================
add_executable(sentinel_engine_server src/main.cpp)
target_link_libraries(sentinel_engine_server PRIVATE sentinel_engine)

# ============================================================================
# Tests
# ============================================================================
if(SENTINEL_BUILD_TESTS)
  FetchContent_MakeAvailable(googletest)
  enable_testing()

  add_executable(sentinel_tests
    tests/unit/test_bloom_filter.cpp
    tests/unit/test_inverted_index.cpp
    tests/unit/test_segment.cpp
    tests/unit/test_wal.cpp
    tests/unit/test_spl_lexer.cpp
    tests/unit/test_spl_parser.cpp
    tests/unit/test_compression.cpp
    tests/unit/test_timestamp_extractor.cpp
  )

  target_link_libraries(sentinel_tests PRIVATE
    sentinel_engine
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(sentinel_tests)
endif()
