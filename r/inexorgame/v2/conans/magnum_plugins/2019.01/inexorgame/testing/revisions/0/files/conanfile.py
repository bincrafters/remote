#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os
import shutil


class MagnumConan(ConanFile):
    name = "magnum_plugins"
    version = "2019.01"
    description = "Plugins for the lightweight and modular graphics middleware for games and data visualization"
    topics = ("conan", "magnum", "graphics", "rendering", "3D", "2D", "opengl")
    url = "https://github.com/mosra/magnum-plugins"
    homepage = "https://magnum.graphics"
    author = "a_teammate <madoe3@web.de>"
    license = "MIT"  # Indicates license type of the packaged library; please use SPDX Identifiers https://spdx.org/licenses/
    exports = ["COPYING"]
    exports_sources = ["CMakeLists.txt", "src/*", "!src/*/Test", "package/conan/*", "modules/*"]
    generators = "cmake"
    short_paths = True  # Some folders go out of the 260 chars path length scope (windows)

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_plugins_static": [True, False],
        "with_assimpimporter": [True, False],
        "with_ddsimporter": [True, False],
        "with_devilimageimporter": [True, False],
        "with_drflacaudioimporter": [True, False],
        "with_drwavaudioimporter": [True, False],
        "with_faad2audioimporter": [True, False],
        "with_freetypefont": [True, False],
        "with_harfbuzzfont": [True, False],
        "with_jpegimageconverter": [True, False],
        "with_jpegimporter": [True, False],
        "with_miniexrimageconverter": [True, False],
        "with_openddl": [True, False],
        "with_opengeximporter": [True, False],
        "with_pngimageconverter": [True, False],
        "with_pngimporter": [True, False],
        "with_stanfordimporter": [True, False],
        "with_stbimageconverter": [True, False],
        "with_stbimageimporter": [True, False],
        "with_stbtruetypefont": [True, False],
        "with_stbvorbisaudioimporter": [True, False],
        "with_tinygltfimporter": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_plugins_static": False,
        "with_assimpimporter": False,
        "with_ddsimporter": False,
        "with_devilimageimporter": False,
        "with_drflacaudioimporter": False,
        "with_drwavaudioimporter": False,
        "with_faad2audioimporter": False,
        "with_freetypefont": False,
        "with_harfbuzzfont": False,
        "with_jpegimageconverter": False,
        "with_jpegimporter": False,
        "with_miniexrimageconverter": False,
        "with_openddl": False,
        "with_opengeximporter": False,
        "with_pngimageconverter": False,
        "with_pngimporter": False,
        "with_stanfordimporter": False,
        "with_stbimageconverter": False,
        "with_stbimageimporter": False,
        "with_stbtruetypefont": False,
        "with_stbvorbisaudioimporter": False,
        "with_tinygltfimporter": False
    }

    _source_subfolder = "source_subfolder"
    _build_subfolder = "build_subfolder"

    requires = (
        "magnum/2019.01@inexorgame/testing"
    )

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def configure(self):
        return

    def requirements(self):
        if self.options.with_pngimageconverter or self.options.with_pngimporter:
            self.requires("libpng/1.6.36@bincrafters/stable")
        if self.options.with_jpegimageconverter or self.options.with_jpegimporter:
            self.requires("libjpeg/9c@bincrafters/stable")
        if self.options.with_freetypefont:
            self.requires("freetype/2.9.1@bincrafters/stable")
        # already bundled:
        # if self.options.with_stbimageconverter or \
        #        self.options.with_stbimageimporter or \
        #        self.options.with_stbtruetypefont or \
        #        self.options.with_stbvorbisaudioimporter:
        #    self.requires("stb/20180214@conan/stable")
        if self.options.with_assimpimporter or \
                self.options.with_ddsimporter or \
                self.options.with_devilimageimporter or \
                self.options.with_faad2audioimporter or \
                self.options.with_freetypefont or \
                self.options.with_harfbuzzfont or \
                self.options.with_openddl or \
                self.options.with_opengeximporter or \
                self.options.with_stanfordimporter:
            self.output.warn("Untested option used, dependencies might not be installed through conan.")
            self.output.warn("Might need to be installed manually on your system before.")


    def source(self):
        # Wrap the original CMake file to call conan_basic_setup
        shutil.move("CMakeLists.txt", "CMakeListsOriginal.txt")
        shutil.move(os.path.join("package", "conan", "CMakeLists.txt"), "CMakeLists.txt")

    def _configure_cmake(self):
        cmake = CMake(self)

        def add_cmake_option(option, value):
            var_name = "{}".format(option).upper()
            value_str = "{}".format(value)
            var_value = "ON" if value_str == 'True' else "OFF" if value_str == 'False' else value_str
            cmake.definitions[var_name] = var_value

        for option, value in self.options.items():
            add_cmake_option(option, value)

        # Magnum uses suffix on the resulting 'lib'-folder when running cmake.install()
        # Set it explicitly to empty, else Magnum might set it implicitly (eg. to "64")
        add_cmake_option("LIB_SUFFIX", "")

        add_cmake_option("BUILD_STATIC", not self.options.shared)
        add_cmake_option("BUILD_STATIC_PIC", not self.options.shared and self.options.get_safe("fPIC"))

        cmake.configure(build_folder=self._build_subfolder)

        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs.append(os.path.join("include", "MagnumExternal", "OpenGL"))
        self.cpp_info.includedirs.append(os.path.join("include", "external"))

        # append plugin folders to make collect_libs() find the static plugins
        if self.options.build_plugins_static:
            plugin_subfolders = ["audioimporters", "importers", "fonts", "imageconverters", "fontconverters"]
            for plugin_subfolder in plugin_subfolders:
                self.cpp_info.libdirs.append(os.path.join("lib", "magnum", plugin_subfolder))
                self.cpp_info.libdirs.append(os.path.join("lib", "magnum-d", plugin_subfolder))
        self.cpp_info.libs = tools.collect_libs(self)
