from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import check_min_cppstd


class Nova(ConanFile):
    name = "nova"
    version = "0.8.0"
    package_type = "library"

    license = "BSL"
    author = "Gábor Krisztián Girhiny <gk.project72@gmail.com>"
    url = "https://github.com/GK72/nova-cpp"
    description = "A collection of modern utilities for various tasks"
    topics = ("modern-cpp")

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    exports_sources = "CMakeLists.txt", "libnova/*", "cmake/*", "*.cmake", "Config.cmake.in"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        self.options["fmt"].header_only = True
        self.options["spdlog"].header_only = True

        if self.options.shared:
            self.options.rm_safe("fPIC")

    def validate(self):
        check_min_cppstd(self, "20")

    def requirements(self):
        for req in self.conan_data.get("private-deps", []):
            self.requires(req)

        for req in self.conan_data.get("public-deps", []):
            self.requires(req, transitive_headers=True, transitive_libs=True)

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["nova"]
