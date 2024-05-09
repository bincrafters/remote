PROJECT(conanefence)
cmake_minimum_required(VERSION 2.8)
include(../conanbuildinfo.cmake)
CONAN_BASIC_SETUP()
if (UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

include("CMakeListsOriginal.cmake")