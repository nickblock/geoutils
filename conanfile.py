import os
from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy, get
from conan.tools.microsoft import is_msvc

required_conan_version = ">=1.53.0"

class GeoUtilsConan(ConanFile):

    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"


    def requirements(self):
        self.requires("gtest/1.14.0")

    def layout(self):
        cmake_layout(self)