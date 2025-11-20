from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy


class JizhakRecipe(ConanFile):
    name = "jizhak"
    version = "1.0.0"
    license = "MIT"
    author = "Qulow (Radomyr B.)"
    url = "https://github.com/BRamil0/Jizhak"
    description = "C++23 library with modules support"

    settings = "os", "compiler", "build_type", "arch"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("fmt/[*]")
        self.requires("boost/[*]")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "23"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

        copy(self, "*.cppm", self.source_folder, self.package_folder + "/src")

    def package_info(self):
        self.cpp_info.libs = ["Jizhak"]

        self.cpp_info.cxxflags = ["-std=c++23"]
        if self.settings.compiler == "msvc":
            self.cpp_info.cxxflags = ["/std:c++latest"]