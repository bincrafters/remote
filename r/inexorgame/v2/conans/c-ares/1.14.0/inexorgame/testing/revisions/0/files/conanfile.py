from conans import ConanFile, tools, CMake
import shutil

class caresConan(ConanFile):
    name = "c-ares"
    version = "1.14.0"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    exports = ["FindCARES.cmake"]
    exports_sources = ["CMakeLists.txt"]
    url = "https://github.com/Croydon/conan-c-ares"
    license = "https://c-ares.haxx.se/license.html"
    description = "A C library for asynchronous DNS requests"
    generators = "cmake"
    ZIP_FOLDER_NAME = "c-ares-cares-{}".format(version.replace(".", "_"))

    def source(self):
        zip_name = "cares-{}.tar.gz".format(self.version.replace(".", "_"))
        tools.get("https://github.com/c-ares/c-ares/archive/{}".format(zip_name), destination=".")
        shutil.move(self.ZIP_FOLDER_NAME, "cares")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["CMAKE_BUILD_TYPE"] = "DEBUG" if self.settings.build_type == "Debug" else "RELEASE"
        cmake.definitions["CARES_STATIC"] = "OFF" if self.options.shared else "ON"
        cmake.definitions["CARES_SHARED"] = "ON" if self.options.shared else "OFF"
        cmake.definitions["CARES_STATIC_PIC"] = "ON"
        cmake.definitions["CARES_INSTALL"] = "ON"
        cmake.configure()
        cmake.build()

    def package(self):
        self.copy("FindCARES.cmake", dst=".", src=".", keep_path=False)
        self.copy("c-ares-config.cmake", dst=".", src="cares", keep_path=False)
        self.copy("ares_build.h", dst="include", src="cares", keep_path=False)
        self.copy("ares_config.h", dst="include", src="cares", keep_path=False)
        self.copy(pattern="*.h", dst="include", src="cares", keep_path=False)
        self.copy("*LICENSE*", dst="", keep_path=False)

        # Copying static and dynamic libs
        self.copy(pattern="*.dll", dst="lib", src="bin", keep_path=False)
        self.copy(pattern="*.dylib", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.lib", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.so*", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.a", dst="lib", src="lib", keep_path=False)

        self.copy("*", dst="lib/cmake/c-ares", src="cares/CMakeFiles/Export/lib/cmake/c-ares")
        self.copy("*", dst="lib/cmake/c-ares", src="cares/CMakeFiles/Export/lib64/cmake/c-ares")

        tools.replace_in_file("{}/c-ares-config.cmake".format(self.package_folder), '''get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)''', '''get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)''')

        tools.replace_in_file("{}/c-ares-config.cmake".format(self.package_folder), '''include("${CMAKE_CURRENT_LIST_DIR}/c-ares-targets.cmake")''', '''include("${CMAKE_CURRENT_LIST_DIR}/lib/cmake/c-ares/c-ares-targets.cmake")''')

        # on Fedora 64-bit c-ares builds the lib in the directory lib/ but the generatore cmake file is searching for it
        tools.replace_in_file("{}/lib/cmake/c-ares/c-ares-targets-release.cmake".format(self.package_folder), "lib64/", "lib/", strict=False)

    def package_info(self):
        self.cpp_info.libdirs = ["lib", "lib64"]
        self.cpp_info.libs.append("cares")
        if not self.options.shared:
            self.cpp_info.defines.append("CARES_STATICLIB")
        # if self.settings.os == "Windows":
            # self.cpp_info.libs[0] += "d"
