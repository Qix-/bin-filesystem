project (filesystem)
cmake_minimum_required (VERSION 2.8.4)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	# Enable C++11 mode on GCC / Clang
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -U__STRICT_ANSI__")
endif ()

add_executable (path_demo
	path_demo.cpp
	filesystem.hpp)

if (BUILD_TESTING)
	enable_testing ()
	add_test (tests path_demo)
endif ()
