cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(pugixml)
set(TARGET_NAME pugixml)


# Setup Conan
include("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
conan_basic_setup()


file(GLOB_RECURSE SOURCES "pugixml/src/*.cpp")

add_library(${TARGET_NAME} ${SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE "pugixml/src")
