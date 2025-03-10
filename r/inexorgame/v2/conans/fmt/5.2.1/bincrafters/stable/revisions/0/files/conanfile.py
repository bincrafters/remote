#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os


class FmtConan(ConanFile):
    name = "fmt"
    version = "5.2.1"
    homepage = "https://github.com/fmtlib/fmt"
    description = "A safe and fast alternative to printf and IOStreams."
    url = "https://github.com/bincrafters/conan-fmt"
    author = "Bincrafters <bincrafters@gmail.com>"
    license = "MIT"
    exports = ['LICENSE.md']
    exports_sources = ['CMakeLists.txt']
    generators = 'cmake'
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "header_only": [True, False], "fPIC": [True, False], "with_fmt_alias": [True, False]}
    default_options = {"shared": False, "header_only": False, "fPIC": True, "with_fmt_alias": False}
    _source_subfolder = "source_subfolder"
    _build_subfolder = "build_subfolder"

    def build_requirements(self):
       if not tools.which("cmake"):
           self.build_requires("cmake_installer/[>3.1]@conan/stable")

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.remove("fPIC")

    def configure(self):
        if self.options.header_only:
            self.settings.clear()
            self.options.remove("shared")
            self.options.remove("fPIC")

    def source(self):
        tools.get("{0}/archive/{1}.tar.gz".format(self.homepage, self.version))
        extracted_dir = self.name + "-" + self.version
        os.rename(extracted_dir, self._source_subfolder)

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["FMT_DOC"] = False
        cmake.definitions["FMT_TEST"] = False
        cmake.definitions["FMT_INSTALL"] = True
        cmake.definitions["FMT_LIB_DIR"] = "lib"
        cmake.configure(build_folder=self._build_subfolder)
        return cmake

    def build(self):
        if not self.options.header_only:
            cmake = self._configure_cmake()
            cmake.build()

    def package_id(self):
        # FMT_STRING_ALIAS is only definition, so it doesn't affect package id
        del self.info.options.with_fmt_alias
        if self.options.header_only:
            self.info.header_only()

    def package(self):
        self.copy("LICENSE.rst", dst="licenses", src=self._source_subfolder, keep_path=False)
        if self.options.header_only:
            src_dir = os.path.join(self._source_subfolder, "src")
            header_dir = os.path.join(self._source_subfolder, "include")
            dst_dir = os.path.join("include", "fmt")
            self.copy("*.h", dst="include", src=header_dir)
            self.copy("*.cc", dst=dst_dir, src=src_dir)
        else:
            cmake = self._configure_cmake()
            cmake.install()

    def package_info(self):
        if self.options.with_fmt_alias:
            self.cpp_info.defines.append("FMT_STRING_ALIAS=1")
        if self.options.header_only:
            self.cpp_info.defines = ["FMT_HEADER_ONLY"]
        else:
            self.cpp_info.libs = tools.collect_libs(self)
            if self.options.shared:
                self.cpp_info.defines.append('FMT_SHARED')
                self.cpp_info.bindirs.append("lib")
