from conans import ConanFile
import os, shutil
from conans.tools import download, unzip, replace_in_file
from conans import CMake


class ElectricFenceConan(ConanFile):
    name = "electric-fence"
    version = "2.2.0"
    branch = "master"
    ZIP_FOLDER_NAME = "electric-fence-%s" % branch
    generators = "cmake"
    settings =  "os", "compiler", "arch", "build_type"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    exports = ["CMakeLists.txt"]
    url="http://github.com/lasote/conan-electric-fence"
    license="GNU GENERAL PUBLIC LICENSE Version 2"

    def config(self):
        try: # Try catch can be removed when conan 0.8 is released
            del self.settings.compiler.libcxx
        except: 
            pass

    def source(self):
        zip_name = "%s.zip" % self.branch
        download("https://github.com/lasote/electric-fence/archive/%s" % zip_name, zip_name)
        unzip(zip_name)
        os.unlink(zip_name)

    def build(self):
        """ Define your project building. You decide the way of building it
            to reuse it later in any other project.
        """
        shutil.move("%s/CMakeLists.txt" % self.ZIP_FOLDER_NAME, "%s/CMakeListsOriginal.cmake" % self.ZIP_FOLDER_NAME)
        shutil.move("CMakeLists.txt", "%s/CMakeLists.txt" % self.ZIP_FOLDER_NAME)
        
        
        cmake = CMake(self.settings)

        self.run("mkdir -p %s/_build"  % self.ZIP_FOLDER_NAME)
        cd_build = "cd %s/_build" % self.ZIP_FOLDER_NAME
        shared = "-DBUILD_SHARED_LIBS=ON" if self.options.shared else ""
        self.run('%s && cmake .. %s %s' % (cd_build, cmake.command_line, shared))
        self.run("%s && cmake --build . %s" % (cd_build, cmake.build_config))

    def package(self):
        # Copying zlib.h, zutil.h, zconf.h
        self.copy("*.h", "include", "%s" % (self.ZIP_FOLDER_NAME), keep_path=False)

        self.copy(pattern="*.so*", dst="lib", src=self.ZIP_FOLDER_NAME, keep_path=False)
        self.copy(pattern="*.a", dst="lib", src="%s/_build" % self.ZIP_FOLDER_NAME, keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ['efence']
        if self.settings.os == "Linux":
            self.cpp_info.libs.append("pthread")
