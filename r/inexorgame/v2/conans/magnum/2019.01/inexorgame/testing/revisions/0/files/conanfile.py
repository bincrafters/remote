#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
import os
import shutil


def sort_libs(correct_order, libs, lib_suffix='', reverse_result=False):
    # Add suffix for correct string matching
    correct_order[:] = [s.__add__(lib_suffix) for s in correct_order]

    result = []
    for expectedLib in correct_order:
        for lib in libs:
            if expectedLib == lib:
                result.append(lib)

    # append all libs not in correct_order to the end, so their not forgotten.
    for lib in libs:
        if lib not in correct_order:
            result.append(lib)

    if reverse_result:
        # Linking happens in reversed order
        result.reverse()

    return result


class MagnumConan(ConanFile):
    name = "magnum"
    version = "2019.01"
    description = "Lightweight and modular graphics middleware for games and data visualization"
    topics = ("conan", "magnum", "graphics", "rendering", "3D", "2D", "opengl")
    url = "https://github.com/mosra/magnum"
    homepage = "https://magnum.graphics"
    author = "helmesjo <helmesjo@gmail.com>"
    license = "MIT"  # Indicates license type of the packaged library; please use SPDX Identifiers https://spdx.org/licenses/
    exports = ["COPYING"]
    exports_sources = ["CMakeLists.txt", "src/*", "!src/*/*/Test", "package/conan/*", "modules/*"]
    generators = "cmake"
    short_paths = True  # Some folders go out of the 260 chars path length scope (windows)

    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_deprecated": [True, False],
        "build_multithreaded": [True, False],
        "build_plugins_static": [True, False],
        "target_gl": [True, False],
        "target_gles": [True, False],
        "target_vk": [True, False],
        "with_anyaudioimporter": [True, False],
        "with_anyimageconverter": [True, False],
        "with_anyimageimporter": [True, False],
        "with_anysceneimporter": [True, False],
        "with_audio": [True, False],
        "with_debugtools": [True, False],
        "with_distancefieldconverter": [True, False],
        "with_eglcontext": [True, False],
        "with_fontconverter": [True, False],
        "with_glfwapplication": [True, False],
        "with_glxapplication": [True, False],
        "with_glxcontext": [True, False],
        "with_gl_info": [True, False],
        "with_imageconverter": [True, False],
        "with_magnumfont": [True, False],
        "with_magnumfontconverter": [True, False],
        "with_meshtools": [True, False],
        "with_objimporter": [True, False],
        "with_opengltester": [True, False],
        "with_primitives": [True, False],
        "with_scenegraph": [True, False],
        "with_sdl2application": [True, False],
        "with_shaders": [True, False],
        "with_text": [True, False],
        "with_tgaimageconverter": [True, False],
        "with_tgaimporter": [True, False],
        "with_vk": [True, False],
        "with_wavaudioimporter": [True, False],
        "with_windowlesseglapplication": [True, False],
        "with_windowlessglxapplication": [True, False],
        "with_xeglapplication": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_deprecated": True,
        "build_multithreaded": True,
        "build_plugins_static": False,
        "target_gl": True,
        "target_gles": False,
        "target_vk": False,
        "with_anyaudioimporter": False,
        "with_anyimageconverter": False,
        "with_anyimageimporter": False,
        "with_anysceneimporter": False,
        "with_audio": False,
        "with_debugtools": True,
        "with_distancefieldconverter": False,
        "with_eglcontext": False,
        "with_fontconverter": False,
        "with_glfwapplication": False,
        "with_glxapplication": False,
        "with_glxcontext": False,
        "with_gl_info": False,
        "with_imageconverter": False,
        "with_magnumfont": False,
        "with_magnumfontconverter": False,
        "with_meshtools": True,
        "with_objimporter": False,
        "with_opengltester": False,
        "with_primitives": True,
        "with_scenegraph": True,
        "with_sdl2application": True,
        "with_shaders": True,
        "with_text": True,
        "with_tgaimageconverter": False,
        "with_tgaimporter": False,
        "with_vk": False,
        "with_wavaudioimporter": False,
        "with_windowlesseglapplication": False,
        "with_windowlessglxapplication": False,
        "with_xeglapplication": False,
    }

    _source_subfolder = "source_subfolder"
    _build_subfolder = "build_subfolder"

    requires = (
        "corrade/2019.01@magnum/stable"
    )

    def system_requirements(self):
        # Install required OpenGL stuff on linux
        packages = []
        if tools.os_info.is_linux:
            if tools.os_info.with_apt:
                installer = tools.SystemPackageTool()

                if self.settings.arch == "x86":
                    arch_suffix = ':i386'
                elif self.settings.arch == "x86_64":
                    arch_suffix = ':amd64'
                elif self.settings.arch == "armv6" or self.settings.arch == "armv7":
                    arch_suffix = ':armel'
                elif self.settings.arch == "armv7hf":
                    arch_suffix = ':armhf'
                elif self.settings.arch == "armv8":
                    arch_suffix = ':arm64'

                if self.options.target_gl:
                    packages.append("libgl1-mesa-dev")
                if self.options.target_gles:
                    packages.append("libgles1-mesa-dev")
                if self.options.target_vk:
                    packages.append("libvulkan-dev")

            elif tools.os_info.with_yum:
                installer = tools.SystemPackageTool()
                if self.settings.arch == "x86":
                    arch_suffix = '.i686'
                elif self.settings.arch == 'x86_64':
                    arch_suffix = '.x86_64'

                packages = []
                if self.options.target_gl:
                    packages.append("mesa-libGL-devel")
                if self.options.target_gles:
                    packages.append("mesa-libGLES-devel")
                if self.options.target_vk:
                    packages.append("vulkan-devel")

            else:
                self.output.warn("Could not determine package manager, skipping Linux system requirements installation.")

        for package in packages:
            installer.install("{}{}".format(package, arch_suffix))

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def configure(self):
        self.options['corrade'].add_option('build_deprecated', self.options.build_deprecated)

        # To fix issue with resource management, see here:
        # https://github.com/mosra/magnum/issues/304#issuecomment-451768389
        if self.options.shared:
            self.options['corrade'].add_option('shared', True)

    def requirements(self):
        if self.options.with_sdl2application:
            self.requires("sdl2/2.0.9@bincrafters/stable")
        if self.options.with_glfwapplication:
            self.requires("glfw/3.2.1.20180327@bincrafters/stable")
        if self.options.with_audio:
            self.requires("openal/1.19.0@bincrafters/stable")

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

    def _append_plugins_folders_to_cpp_info(self):
        """ Append the needed plugins to the cpp_info object """

        plugin_subfolders = ["audioimporters", "importers", "fonts", "imageconverters", "fontconverters"]
        for plugin_subfolder in plugin_subfolders:
            self.cpp_info.libdirs.append(os.path.join("lib", "magnum", plugin_subfolder))
            self.cpp_info.libdirs.append(os.path.join("lib", "magnum-d", plugin_subfolder))


    def package_info(self):
        # fix issue on Windows and OSx not finding the KHR files
        self.cpp_info.includedirs.append(os.path.join("include", "MagnumExternal", "OpenGL"))

        # append plugin folders to make static plugins happy
        if self.options.build_plugins_static:
            plugin_subfolders = ["audioimporters", "importers", "fonts", "imageconverters", "fontconverters"]
            for plugin_subfolder in plugin_subfolders:
                self.cpp_info.libdirs.append(os.path.join("lib", "magnum", plugin_subfolder))
                self.cpp_info.libdirs.append(os.path.join("lib", "magnum-d", plugin_subfolder))

        # See dependency order here: https://doc.magnum.graphics/magnum/custom-buildsystems.html
        allLibs = [
            # 1
            "Magnum",
            "MagnumAnimation",
            "MagnumMath",
            # 2
            "MagnumAudio",
            "MagnumGL",
            "MagnumSceneGraph",
            "MagnumTrade",
            "MagnumVk",
            # 3
            "MagnumMeshTools",
            "MagnumPrimitives",
            "MagnumShaders",
            "MagnumTextureTools",
            "MagnumGlfwApplication",
            "MagnumXEglApplication",
            "MagnumWindowlessEglApplication",
            "MagnumGlxApplication",
            "MagnumWindowlessGlxApplication",
            "MagnumSdl2Application",
            "MagnumWindowlessSdl2Application",
            # 4
            "MagnumDebugTools",
            "MagnumOpenGLTester",
            "MagnumText",
        ]

        # Sort all built libs according to above, and reverse result for correct link order
        suffix = '-d' if self.settings.build_type == "Debug" else ''
        builtLibs = tools.collect_libs(self)
        self.cpp_info.libs = sort_libs(correct_order=allLibs, libs=builtLibs, lib_suffix=suffix, reverse_result=True)

        if self.settings.os == "Windows":
            if self.settings.compiler == "Visual Studio":
                if not self.options.shared:
                    self.cpp_info.libs.append("OpenGL32.lib")
            else:
                self.cpp_info.libs.append("opengl32")
        else:
            if self.settings.os == "Macos":
                self.cpp_info.exelinkflags.append("-framework OpenGL")
            elif not self.options.shared:
                self.cpp_info.libs.append("GL")
