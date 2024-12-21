cmake_minimum_required(VERSION 3.10)
project(wotr)
set(CMAKE_CXX_STANDARD 17)
include_directories("${PROJECT_SOURCE_DIR}")

add_library(wotr STATIC "wotr.cc")
add_library(wotr_shared SHARED "wotr.cc")
