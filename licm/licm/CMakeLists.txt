cmake_minimum_required(VERSION 3.15)
project(LicmPass LANGUAGES CXX)

find_package(LLVM 14 REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_library(LicmPass MODULE LicmPass.cpp)
set_target_properties(LicmPass PROPERTIES CXX_STANDARD 17 PREFIX "")
target_include_directories(LicmPass PRIVATE ${LLVM_INCLUDE_DIRS})