from conans import ConanFile, CMake, tools
import os

class CppreactConan(ConanFile):
    name = "cpp.react"
    version = "legacy1"
    license = "BSL-1.0"
    author = "a_teammate"
    url = "https://github.com/inexorgame/conan-cpp.react"
    description = "A reactive programming library for C++11"
    topics = ("reactive", "observable", "observer", "flow based")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    requires = "TBB/2019_U3@conan/stable"
    generators = "cmake"

    def source(self):
        self.run("git clone https://github.com/inexorgame/cpp.react --depth 1")
        cmake_file = os.path.join("cpp.react", "CMakeLists.txt")
        tools.replace_in_file(cmake_file, "project (CppReact)",
                              '''project (CppReact)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()''')

    def build(self):
        cmake = CMake(self)
        cmake.definitions["build_examples"] = False
        cmake.configure(source_folder="cpp.react")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src=os.path.join("cpp.react", "include"))
        self.copy("*cppreact.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["CppReact"]

