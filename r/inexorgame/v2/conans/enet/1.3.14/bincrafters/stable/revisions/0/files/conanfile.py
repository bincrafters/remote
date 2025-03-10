#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os


class EnetConan(ConanFile):
    name = "enet"
    version = "1.3.14"
    description = "ENet reliable UDP networking library"
    topics = ("conan", "enet", "udp", "networking")
    url = "https://github.com/bincrafters/conan-enet"
    homepage = "https://github.com/lsalzman/enet"
    author = "Bincrafters <bincrafters@gmail.com>"
    license = "MIT"
    exports = ["LICENSE.md"]
    exports_sources = ["CMakeLists.txt"]
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = {'shared': 'False'}
    _source_subfolder = "source_subfolder"

    def config(self):
        del self.settings.compiler.libcxx  # We are a pure C lib.

    def source(self):
        source_url = "http://enet.bespin.org/download"
        sha256 = "98f6f57aab0a424469619ed3047728f0d3901ce8f0dea919c11e7966d807e870"
        tools.get("{0}/{1}-{2}.tar.gz".format(source_url, self.name, self.version), sha256=sha256)
        extracted_dir = self.name + "-" + self.version
        os.rename(extracted_dir, self._source_subfolder)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        include_folder = os.path.join(self._source_subfolder, "include")
        self.copy(pattern="*", dst="include", src=include_folder)
        self.copy(pattern="*.lib", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.a", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.dylib", dst="lib", src="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.libs.append("ws2_32")
            self.cpp_info.libs.append("winmm")
