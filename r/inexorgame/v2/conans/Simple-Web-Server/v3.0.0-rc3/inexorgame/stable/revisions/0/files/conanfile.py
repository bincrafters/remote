#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, tools
import os


class LibnameConan(ConanFile):
    name = "Simple-Web-Server"
    version = "v3.0.0-rc3"
    description = "A very simple, fast, multithreaded, platform independent HTTP and HTTPS server and client library implemented using C++11 and Asio (both Boost.Asio and standalone Asio can be used). Created to be an easy way to make REST resources available from C++ applications."
    # topics can get used for searches, GitHub topics, Bintray tags etc. Add here keywords about the library
    topics = ("c++11", "http", "rest")
    url = "https://github.com/inexorgame/conan-simple-web-server"
    homepage = "https://gitlab.com/eidheim/Simple-Web-Server"
    author = "Ole Christian Eidheim <eidheim@gmail.com>"
    license = "MIT"  # Indicates license type of the packaged library; please use SPDX Identifiers https://spdx.org/licenses/
    no_copy_source = True

    # Packages the license for the conanfile.py
    exports = ["LICENSE.md"]

    # Custom attributes for Bincrafters recipe conventions
    _source_subfolder = "Simple-Web-Server"

    requires = (
        "asio/1.12.2@bincrafters/stable"
    )

    def source(self):
        git = tools.Git(self._source_subfolder)
        git.clone("https://gitlab.com/eidheim/Simple-Web-Server.git")
        git.checkout("v3.0.0-rc3")


    def package(self):
        self.copy(pattern="LICENSE", dst="licenses", src=self._source_subfolder)
        self.copy(pattern="*.hpp", dst="include", src=self._source_subfolder)


    def package_info(self):
        self.cpp_info.defines = ["USE_STANDALONE_ASIO"]

    def package_id(self):
        self.info.header_only()
