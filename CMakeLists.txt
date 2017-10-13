cmake_minimum_required(VERSION 2.8.4)
project(path)

# C++11 required for this project
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(COMPILER_IS_CLANG ON)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
else()
  set(COMPILER_IS_CLANG OFF)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(COMPILER_IS_GCC ON)
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0.0)
    message(FATAL_ERROR "<codecvt> support was introduced in GCC 5.")
  endif()
else()
  set(COMPILER_IS_GCC OFF)
endif()

if (COMPILER_IS_CLANG OR COMPILER_IS_GCC)
  # Add strict ansi for __f128
  set(CMAKE_CXX_FLAGS "-D__STRICT_ANSI__")
endif()

add_executable(path_demo
	path_demo.cpp
	filesystem.hpp
)
enable_testing()
add_test(tests path_demo)
