from conans import ConanFile, CMake, tools
import os


class NasmConan(ConanFile):
    name = "nasm"
    version = "2.12.02"
    license = "LGPL"
    url = "https://github.com/lasote/conan-nasm-installer"
    settings = {"os": ["Windows", "Macos"], "arch": {"x86", "x86_64"}}
    build_policy = "missing"
    
    @property
    def nasm_url_id(self):
        nasm_os_url_id = "" #nasm url identifier
        if self.settings.arch == "x86":
            nasm_os_url_id = "win32"
        else:
            nasm_os_url_id = "win64" 
        
        if self.settings.os == "Macos":
            nasm_os_url_id = "macosx"
            
        return nasm_os_url_id
    
    @property
    def nasm_folder_name(self):
        return "nasm-%s" % self.version

    def source(self):
        nasm_zip_name = "%s-%s.zip" % (self.nasm_folder_name, self.nasm_url_id)
        tools.download("http://www.nasm.us/pub/nasm/releasebuilds/%s/%s/%s" % (self.version, self.nasm_url_id, nasm_zip_name), nasm_zip_name)
        self.output.warn("Downloading nasm: http://www.nasm.us/pub/nasm/releasebuilds/%s/%s/%s" % (self.version, self.nasm_url_id, nasm_zip_name))
        tools.unzip(nasm_zip_name)
        os.unlink(nasm_zip_name)

    def build(self):
        pass

    def package(self):
        self.copy("*", dst="", src=self.nasm_folder_name, keep_path=True)

    def package_info(self):
        nasm_path = os.path.join(self.package_folder)
        self.env_info.path.append(nasm_path)
