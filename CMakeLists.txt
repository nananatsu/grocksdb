# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug  -DROCKSDB=rocksdb_path
# cmake --build build
cmake_minimum_required(VERSION 3.14)
project(grocksdb)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
add_definitions(-DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(extend extend.cc extend_inner.cc)
target_link_libraries(extend PRIVATE "-L${ROCKSDB} -lrocksdb -lstdc++ -lm -lz -lsnappy -llz4 -lzstd -lbz2")
target_compile_options(extend PRIVATE "-I${ROCKSDB}/include" "-I${ROCKSDB}")

add_executable(
  extend_test
  extend_test.cc
  extend_inner_test.cc
)
target_link_libraries(
  extend_test
  GTest::gtest_main 
  extend
)
target_compile_options(extend_test PRIVATE "-I${ROCKSDB}/include")

include(GoogleTest)
gtest_discover_tests(extend_test)