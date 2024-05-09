cmake_minimum_required(VERSION 3.1)

project(caresConan)

include(conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_subdirectory(cares)
