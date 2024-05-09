cmake_minimum_required(VERSION 2.8.12)

# Project Name and start working in project scope (some things just work until now, some just work after this command)
project(SDL2_image C)

# Add conan dependencies
include(../conanbuildinfo.cmake) # TODO This is actually ugly isnt it? Since it only works inside a "require", not using a normal "mkdir build && cd build && conan install .. && cmake .."
conan_basic_setup()


if(APPLE)
  option(USE_IMAGEIO "use native Mac OS X frameworks for loading images" ON)
  if(USE_IMAGEIO)
    set(IMAGEIO_SOURCE "IMG_ImageIO.m")
  endif()
endif()

# Type beeing upper case
macro (add_image_option type)
  option(LOAD_${type} "${type} images support" ON)
  if(LOAD_${type})
    add_definitions("-DLOAD_${type}")
    if(BUILD_SHARED_LIBS)
      add_definitions("-DLOAD_${type}_DYNAMIC") # This is actually only valid for jpg, webp, tif and png.. but doesnt hurt otherwise
      # @lasote: should we make that options based on the settings of the requirements?
    endif()
  endif()
endmacro()

add_image_option(BMP)
add_image_option(GIF)
add_image_option(JPG)
add_image_option(LBM)
add_image_option(PCX)
add_image_option(PNG)
add_image_option(PNM)
add_image_option(TGA)
add_image_option(TIF)
add_image_option(WEBP)
add_image_option(XCF)
add_image_option(XV)

if(LOAD_JPG)
  option(FAST_JPG_LOAD "Define this for faster but less quality jpeg loading" OFF)
  if(FAST_JPG_LOAD)
    add_definitions("-DFAST_JPEG -DFAST_IS_JPEG")
  endif()
endif()

set(SOURCE_FILES
IMG.c
IMG_bmp.c
IMG_gif.c
IMG_jpg.c
IMG_lbm.c
IMG_pcx.c
IMG_png.c
IMG_pnm.c
IMG_tga.c
IMG_tif.c
IMG_xcf.c
IMG_xpm.c
IMG_xv.c
IMG_webp.c
${IMAGEIO_SOURCE}
miniz.h
CACHE INTERNAL "")

add_library(SDL2_image ${SOURCE_FILES})

if(FPIC)
    MESSAGE(STATUS "ENABLED FPIC")
    SET_TARGET_PROPERTIES(SDL2_image PROPERTIES COMPILE_FLAGS "-fPIC")
endif()
