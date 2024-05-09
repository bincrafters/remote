#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os


class SpdlogConan(ConanFile):
    name = "spdlog"
    version = "1.3.1"
    description = "Fast C++ logging library"
    url = "https://github.com/bincrafters/conan-spdlog"
    homepage = "https://github.com/gabime/spdlog"
    author = "Bincrafters <bincrafters@gmail.com>"
    topics = ("conan", "spdlog", "logging", "header-only")
    license = "MIT"
    exports = ["LICENSE.md"]
    exports_sources = ["CMakeLists.txt"]
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"
    _source_subfolder = "source_subfolder"

    def requirements(self):
        self.requires("fmt/5.3.0@bincrafters/stable")

    def source(self):
        tools.get("{0}/archive/v{1}.tar.gz".format(self.homepage, self.version))
        extracted_dir = self.name + "-" + self.version
        os.rename(extracted_dir, self._source_subfolder)

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["SPDLOG_BUILD_EXAMPLES"] = False
        cmake.definitions["SPDLOG_BUILD_TESTING"] = False
        cmake.definitions["SPDLOG_BUILD_BENCH"] = False
        cmake.definitions["SPDLOG_FMT_EXTERNAL"] = True
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst='licenses', src=self._source_subfolder)
        cmake = self._configure_cmake()
        cmake.install()
        tools.replace_in_file(os.path.join(self.package_folder, "lib", "cmake", "spdlog", "spdlogConfigVersion.cmake"),
                              '# check',
                              'return() #')


    def package_info(self):
        self.cpp_info.defines.append("SPDLOG_FMT_EXTERNAL")
        if self.settings.os == "Linux":
            self.cpp_info.libs.append("pthread")

    def package_id(self):
        self.info.header_only()
