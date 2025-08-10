from conans import ConanFile, CMake, tools

class JizhakConan(ConanFile):
    name = "Jizhak"
    version = "1.0.0"
    license = "MIT"
    author = "Qulow (Radomyr B.) <qulowg@gmail.com>"
    url = "https://github.com/BRamil0/Jizhak"
    description = "Your C++ library description"
    topics = ("modules", "cpp23", "fmt", "boost")
    settings = "io"
    generators = "cmake", "cmake_find_package"
    exports_sources = "src/*", "include/*", "CMakeLists.txt"

    requires = (
        "fmt/11.2.0",
        "boost/1.88.0"
    )

    options = {
        "shared": [True, False]
    }
    default_options = {
        "shared": False,
        "boost:shared": False,
        "fmt:shared": False
    }

    _cmake = None

    def configure(self):
        if self.options.shared:
            self.options["boost"].shared = True
            self.options["fmt"].shared = True

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake

        self._cmake = CMake(self)
        self._cmake.definitions["CMAKE_CXX_STANDARD"] = "23"
        self._cmake.definitions["CMAKE_CXX_STANDARD_REQUIRED"] = "ON"
        self._cmake.configure()
        return self._cmake

    def package(self):
        self.copy("*.hpp", dst="include", src="include")
        self.copy("*.h", dst="include", src="include")
        self.copy("*jizhak.lib", dst="lib", keep_path=False)
        self.copy("*jizhak.a", dst="lib", keep_path=False)
        self.copy("*jizhak.dll", dst="bin", keep_path=False)
        self.copy("*jizhak.so", dst="lib", keep_path=False)
        self.copy("*jizhak.dylib", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["jizhak"]
