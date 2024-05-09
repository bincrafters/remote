import os
from conans import ConanFile, CMake, tools

class RestbedConan(ConanFile):
    name = 'restbed'
    version = '6eb385fa9051203f28bf96cc1844bbb5a9a6481f'
    license = 'AGPL'
    description = 'RESTful server framework for C++11 applications'
    url = 'https://github.com/Corvusoft/restbed'
    settings = 'os', 'compiler', 'build_type', 'arch'
    options = {
        'shared': [True, False],
        'ssl': [True, False]
    }
    default_options = 'shared=False','ssl=False'
    requires = 'asio/1.12.0@bincrafters/stable'
    generators = 'cmake'

    def source(self):
        if 'CONAN_RUNNER_ENCODED' in os.environ:    #conan package tools linux docker build
            os.environ['GIT_SSL_NO_VERIFY'] = 'true'
        self.run('git clone --branch master https://github.com/Corvusoft/restbed && cd restbed && git checkout {}'.format(self.version))
        tools.replace_in_file('restbed/CMakeLists.txt', 'project( "restbed" VERSION 4.7.0 LANGUAGES CXX )', '''project( "restbed" VERSION 4.7.0 LANGUAGES CXX )
            include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
            conan_basic_setup()''')
        # remove any testing
        tools.replace_in_file('restbed/CMakeLists.txt', 'enable_testing( )', '''if(0)
 enable_testing( )''')
        tools.replace_in_file('restbed/CMakeLists.txt', 'add_subdirectory( "${PROJECT_SOURCE_DIR}/test/integration" )', '''add_subdirectory( "${PROJECT_SOURCE_DIR}/test/integration" )
endif()''')
        tools.replace_in_file('restbed/CMakeLists.txt', 'find_package( catch REQUIRED )', '''# find_package( catch REQUIRED )''')

    def requirements(self):
        if self.options.ssl:
            self.requires.add('OpenSSL/1.0.2l@conan/stable', private=False)
        else:
            if 'OpenSSL' in self.requires:
                del self.requires['OpenSSL']

    def build(self):
        shared = '-DBUILD_SHARED=ON' if self.options.shared else '-DBUILD_SHARED=OFF'
        ssl = '-DBUILD_SSL=ON' if self.options.ssl else '-DBUILD_SSL=OFF'
        cmake = CMake(self)
        cmake.configure(source_dir='restbed', build_dir='./', args=[shared, ssl, "-DCMAKE_TESTING_ENABLED=OFF"])
        cmake.build()

    def package(self):
        self.copy('restbed', dst='include', src='restbed/source')
        self.copy('*.hpp', dst='include', src='restbed/source')
        self.copy('librestbed.a', dst='lib', src='lib')
        self.copy('restbed.lib', dst='lib', src='lib')
        self.copy('librestbed.so*', dst='lib', src='lib')
        self.copy('librestbed*dylib', dst='lib', src='lib')
        
    def package_info(self):
        self.cpp_info.libs = ['restbed']
