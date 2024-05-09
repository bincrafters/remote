from os import path
import os
from conans import ConanFile, tools


class GNConanFile(ConanFile):
    name = "GN"
    version = "master"
    description = """GN is a meta-build system that generates build files for Ninja."""
    license = "Google license, variation a BSD-3-clause (https://opensource.org/licenses/bsd-license.php)"
    url = "https://gn.googlesource.com/gn"
    settings = {'os_build': ['Windows', 'Linux', 'Macos'], 'arch_build': ['x86', 'x86_64'], 'compiler': None}
    source_dir = "{name}-{version}".format(name=name, version=version)
    options = {
        "tests": [True, False]
    }
    default_options = {
        "tests": False
    }
    scm = {"revision": "master",
           "subfolder": "GN-master",
           "type": "git",
           "url": "https://gn.googlesource.com/gn"}
    build_requires = "ninja_installer/1.8.2@bincrafters/stable"

    def source(self):
        pass

    def build(self):
        with tools.chdir(self.source_dir):
            build_dir = path.join(self.build_folder, "build-dir")
            python_executable = os.path.join(os.__file__.split("lib/")[0],"bin","python")
            if self.settings.os_build == "Windows":
                build_env = tools.vcvars_dict(self.settings)
            else:
                # GNU binutils's ld and gold don't support Darwin (macOS)
                # Use the default provided linker
                build_env = dict()
                if self.settings.os_build == "Linux":
                    build_env["LDFLAGS"] = "-fuse-ld=gold"

            with tools.environment_append(build_env):
                self.run("{python} build/gen.py --out-path={build_dir}"\
                         .format(python=python_executable, build_dir=build_dir))
                self.run("ninja -j {cpu_nb} -C {build_dir}"\
                         .format(cpu_nb=tools.cpu_count()-1, build_dir=build_dir))
                if self.options.tests:
                    self.run("{build_dir}/gn_unittests".format(build_dir=build_dir))

    def package(self):
        gn_executable = "gn.exe" if self.settings.os_build == "Windows" else "gn"
        self.copy(gn_executable, dst="bin", src="build-dir", keep_path=False)

    def package_info(self):
        # ensure gn is executable
        if str(self.settings.os_build) in ['Linux', 'Macosx']:
            name = os.path.join(self.package_folder, 'bin', 'gn')
            os.chmod(name, os.stat(name).st_mode | 0o111)
        self.env_info.PATH.append(os.path.join(self.package_folder, 'bin'))

    def package_id(self):
        self.info.settings.compiler = 'Any'

    def deploy(self):
        self.copy("*", keep_path=True)
