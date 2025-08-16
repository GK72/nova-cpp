import os

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.build import can_run


class novaTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
        self.options["fmt"].header_only = True
        self.options["spdlog"].header_only = True

    def requirements(self):
        #  self.requires(self.tested_reference_str)
        for req in self.conan_data:
            self.requires(req)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def layout(self):
        cmake_layout(self)

    def test(self):
        if can_run(self):
            cmd = os.path.join(self.cpp.build.bindir, "package-test")
            self.run(cmd, env="conanrun")
