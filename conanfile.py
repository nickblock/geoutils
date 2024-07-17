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
        self.requires("assimp/5.4.1")
        self.requires("poly2tri/cci.20130502")
        self.requires("rapidjson/cci.20230929")
        self.requires("glm/cci.20230113")
