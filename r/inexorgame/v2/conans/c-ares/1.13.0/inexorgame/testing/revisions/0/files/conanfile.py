from conans import ConanFile, AutoToolsBuildEnvironment, tools, CMake
from conans.tools import download, unzip
import os


class caresConan(ConanFile):
    name = "c-ares"
    version = "1.13.0"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    exports = ["FindCARES.cmake"]
    url = "https://github.com/Croydon/conan-c-ares"
    license = "https://c-ares.haxx.se/license.html"
    description = "A C library for asynchronous DNS requests"
    generators = "cmake"
    ZIP_FOLDER_NAME = "c-ares-cares-%s" % version.replace(".", "_")

    def source(self):
        zip_name = "cares-%s.tar.gz" % self.version.replace(".", "_")
        download("https://github.com/c-ares/c-ares/archive/%s" % zip_name, zip_name, verify=True)
        unzip(zip_name)
        os.unlink(zip_name)

    def build(self):
        self.output.info("c-ares build:")
        self.output.info("Shared? %s" % self.options.shared)

        # Use configure && make in linux and Macos, and nmake in windows
        env = AutoToolsBuildEnvironment(self)
        envvars = env.vars

        with tools.environment_append(envvars):
            # the following lines are directly from
            # https://github.com/c-ares/c-ares/blob/b0aebb95152d5871531e1dc3ffb7dd6910c7ec38/travis/build.sh
            # self.run("cd %s" % (self.ZIP_FOLDER_NAME))
            # self.run("mkdir cmakebld && cd cmakebld")
            # builddir = "%s/build" % self.ZIP_FOLDER_NAME

            args = ["-DCMAKE_BUILD_TYPE=%s" % ("DEBUG" if self.settings.build_type == "Debug" else "RELEASE")]
            args += ["-DCARES_STATIC=%s" % ("OFF" if self.options.shared else "ON")]
            args += ["-DCARES_SHARED=%s" % ("ON" if self.options.shared else "OFF")]
            args += ["-DCARES_STATIC_PIC=ON"]

            cmake = CMake(self)

            if self.settings.os == "Linux" or self.settings.os == "Macos":
                args += ['-G "Unix Makefiles"']
                self.run('cmake "%s" %s %s' % (self.ZIP_FOLDER_NAME, cmake.command_line, " ".join(args)))
                self.run("make")
                # self.run("./bin/adig www.google.com")
                # self.run("./bin/acountry www.google.com")
                # self.run("./bin/ahost www.google.com")
                self.run("ls lib/")

            else:
                args += ['-G "NMake Makefiles"']

                self.run('cmake "%s" %s %s' % (self.ZIP_FOLDER_NAME, cmake.command_line, " ".join(args)))
                self.run("nmake")

            # fix path that third-party applications can find c-ares
            tools.replace_in_file("c-ares-config.cmake", '''get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)''', '''get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)''')

            # Generate the cmake options
            # nmake_options = "CFG="
            # nmake_options += "dll-" if self.options.shared else "lib-"
            # nmake_options += "debug" if self.settings.build_type == "Debug" else "release"
            # Check if it must be built using static CRT
            # if(self.settings.compiler.runtime == "MT" or self.settings.compiler.runtime == "MTd"):
            #    nmake_options += " RTLIBCFG=static"

            # command_line_env comes with /. In Windows, \ are used
            # self.output.info(nmake_options)
            # with tools.environment_append(envvars):
                # command = ('cd %s && buildconf.bat && nmake /f Makefile.msvc %s' \
                # % (self.ZIP_FOLDER_NAME, nmake_options))
                # self.run(command)

    def package(self):
        self.copy("FindCARES.cmake", dst=".", src=".", keep_path=False)
        self.copy("c-ares-config.cmake", dst=".", src=".", keep_path=False)
        self.copy("ares_build.h", dst="include", src=".", keep_path=False)
        self.copy("ares_config.h", dst="include", src=".", keep_path=False)
        self.copy(pattern="*.h", dst="include", src=self.ZIP_FOLDER_NAME, keep_path=False)

        # Copying static and dynamic libs
        self.copy(pattern="*.dll", dst="lib", src="bin", keep_path=False)
        self.copy(pattern="*.dylib", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.lib", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.so*", dst="lib", src="lib", keep_path=False)
        self.copy(pattern="*.a", dst="lib", src="lib", keep_path=False)

        cmake_folder = "{}/cmake/c-ares".format(self.get_install_lib_path())
        self.copy("c-ares-targets.cmake", dst=".", src=cmake_folder)
        self.copy("c-ares-targets-{}.cmake".format("debug" if self.settings.build_type == "Debug" else "release"), dst=".", src=cmake_folder)

    def get_install_lib_path(self):
        install_path = "{}/install".format(self.build_folder)
        if os.path.isfile("{}/lib/cmake/c-ares/c-ares-targets.cmake".format(install_path)):
            return "{}/lib".format(install_path)
        elif os.path.isfile("{}/lib64/cmake/c-ares/c-ares-targets.cmake".format(install_path)):
            return "{}/lib64".format(install_path)
        # its "{}/install/{{lib|lib64}}/cmake/gRPC/gRPCTargets.cmake".format(self.build_folder)

    # def package_info(self):
        # Define the libraries
        # if self.settings.os == "Windows":
            # self.cpp_info.libs = ['cares'] if self.options.shared else ['libcares']
            # if self.settings.build_type == "Debug":
            #     self.cpp_info.libs[0] += "d"
            # self.cpp_info.libs.append('Ws2_32')
            # self.cpp_info.libs.append('wsock32')
        # else:
            # pass

        # Definitions for static build
        # if not self.options.shared:
            # self.cpp_info.defines.append("CARES_STATICLIB=1")
