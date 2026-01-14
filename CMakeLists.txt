cmake_minimum_required(VERSION 3.24)

project(TennisForTwo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# raylib (local)
add_subdirectory(external/raylib-5.5)

add_executable(TennisForTwo
  src/main.cpp
)

target_link_libraries(TennisForTwo PRIVATE raylib)

if (WIN32)
  target_compile_definitions(TennisForTwo PRIVATE NOMINMAX)
endif()
