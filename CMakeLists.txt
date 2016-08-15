cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GENERATE_EXAMPLE "Generate example project" ON)

include_directories("src")

if (GENERATE_EXAMPLE)
	project(Example)
	add_executable(Example "example/Main.cpp")
	target_link_libraries(Example RectBinPack)
endif()

project(RectBinPack)
file(GLOB_RECURSE RECTBINPACKSRC "src/*.cpp" "src/*.h")
add_library(RectBinPack STATIC ${RECTBINPACKSRC})
