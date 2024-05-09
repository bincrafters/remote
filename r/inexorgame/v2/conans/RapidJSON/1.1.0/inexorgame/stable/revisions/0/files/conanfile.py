from conans import ConanFile


class RapidJSONConan(ConanFile):
    name = "RapidJSON"
    version = '1.1.0'
    url = 'https://github.com/inexorgame/conan-RapidJSON'
    license = 'MIT'
    exports = 'FindRapidJSON.cmake'

    def source(self):
        self.run("git clone https://github.com/Tencent/rapidjson")
        self.run("cd rapidjson && git checkout v%s" % self.version)

    def package(self):
        self.copy('FindRapidJSON.cmake', '.', '.')
        self.copy('*', dst='include', src='rapidjson/include')

    def build(self):
        pass
