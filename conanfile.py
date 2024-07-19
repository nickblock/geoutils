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

    options = {
        "pbf": [True, False],
        "xml": [True, False],
        "geos": [True, False],
        "gdal": [True, False],
        "proj": [True, False],
        "lz4": [True, False],
    }
    default_options = {
        "pbf": True,
        "xml": True,
        "geos": True,
        "gdal": False,
        "proj": True,
        "lz4": True,
    }
    options_description = {
        # https://github.com/osmcode/libosmium/blob/v2.20.0/cmake/FindOsmium.cmake#L30-L37
        "pbf": "include libraries needed for PBF input and output",
        "xml": "include libraries needed for XML input and output",
        "geos": "include if you want to use any of the GEOS functions",
        "gdal": "include if you want to use any of the OGR functions",
        "proj": "include if you want to use any of the Proj.4 functions",
        "lz4": "include support for LZ4 compression of PBF files",
    }

    def requirements(self):
        self.requires("gtest/1.14.0")
        self.requires("assimp/5.4.1")
        self.requires("poly2tri/cci.20130502")
        self.requires("rapidjson/cci.20230929")
        self.requires("glm/cci.20230113")
        self.requires("s2geometry/0.11.1")
        self.requires("protozero/1.7.1")
        self.requires("eigen/3.4.0")
        if self.options.xml:
            self.requires("expat/[>=2.6.2 <3]")
            self.requires("bzip2/1.0.8")
        if self.options.pbf or self.options.xml:
            self.requires("zlib/[>=1.2.11 <2]")
        if self.options.geos:
            self.requires("geos/3.12.0")
        if self.options.gdal:
            self.requires("gdalcpp/1.3.0")
        if self.options.proj:
            self.requires("proj/9.3.1")
        if self.options.lz4:
            self.requires("lz4/1.9.4")