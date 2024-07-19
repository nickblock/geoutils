cmake_minimum_required(VERSION 3.22.6)

# Name of the project (will be the name of the plugin)
project(geoutils)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
      add_compile_definitions(-DNOMINMAX)
endif()

find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(poly2tri CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(s2 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_path(OSMIUM_INCLUDE_DIRS "osmium/version.hpp")
find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")
find_package(BZip2 REQUIRED)
find_package(EXPAT REQUIRED)
find_package(ZLIB REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(MACOSX TRUE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1 -DGLM_FORCE_RADIANS=1")

if(NOT MSVC)
      if(CMAKE_BUILD_TYPE MATCHES "Debug")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")
      endif()
endif()

option(BUILD_OSMSPLIT "Build osmsplit" ON)

if(BUILD_OSMSPLIT)
      add_library(osmsplitlib STATIC
            osmsplit/osmsplitconfig.cpp
            osmsplit/osmsplitwriter.cpp)

      set(OSMSPLIT_INCLUDES
            ${OSMIUM_INCLUDE_DIRS}
            ${PNGPP_INCLUDE_DIRS}
            ext)
      target_include_directories(osmsplitlib PUBLIC ${OSMSPLIT_INCLUDES})
      target_link_libraries(osmsplitlib PUBLIC rapidjson PNG::PNG BZip2::BZip2 EXPAT::EXPAT)

      add_executable(osmsplit
            osmsplit/main.cpp)

      target_include_directories(osmsplit PUBLIC ${OSMSPLIT_INCLUDES})

      target_link_libraries(osmsplit PUBLIC osmsplitlib)

      if(MACOSX)
            target_include_directories(osmsplit PUBLIC /usr/local/include)
            link_directories(/usr/local/lib)
      endif()
endif()

option(BUILD_OSMS2SPLIT "Build osms2split" ON)

if(BUILD_OSMS2SPLIT)
      add_executable(osms2split
            osms2split/src/main.cpp
            osms2split/src/s2splitter.cpp)

      # target_compile_options(osms2split PRIVATE -Wno-attributes)
      target_include_directories(osms2split PUBLIC ${OSMIUM_INCLUDE_DIRS} ext)
      target_link_libraries(osms2split PUBLIC
            s2::s2
            EXPAT::EXPAT
            BZip2::BZip2
            ZLIB::ZLIB
      )

      if(MACOSX)
            target_include_directories(s2 PUBLIC ext/openssl)
            link_directories(/usr/local/opt/bzip2/lib ${CMAKE_CURRENT_LIST_DIR}/ext/openssl/mac /usr/local/lib)
      elseif(POSIX)
            target_link_libraries(osms2split PUBLIC pthread)
      endif()
endif()

option(BUILD_OSM2ASSIMP "Build osm2assimp" ON)

if(BUILD_OSM2ASSIMP)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPACKAGE_VERSION=\"1\" -fcommon")

      add_library(clipper STATIC
            ext/clipper/clipper.cpp)

      target_include_directories(clipper PRIVATE
            ext/clipper)

      add_library(osm2assimpLib STATIC
            osm2assimp/sceneconstruct.cpp
            osm2assimp/convertlatlng.cpp
            osm2assimp/eigenconversion.cpp
            osm2assimp/assimpwriter.cpp
            osm2assimp/osmfeature.cpp
            osm2assimp/ground.cpp
            osm2assimp/geomconvert.cpp
            osm2assimp/viewfilter.cpp
            osm2assimp/utils.cpp
            ext/common.cpp
      )

      target_include_directories(osm2assimpLib PUBLIC
            ${OSMIUM_INCLUDE_DIRS}
            osm2assimp
            ext/clipper
            ext
            s2util
      )
      target_link_libraries(osm2assimpLib PUBLIC
            assimp::assimp
            poly2tri::poly2tri
            glm::glm
            s2::s2
            Eigen3::Eigen
            EXPAT::EXPAT
            BZip2::BZip2
            ZLIB::ZLIB
      )

      add_executable(osm2assimp
            osm2assimp/main.cpp
      )

      target_include_directories(osm2assimp PUBLIC
            osm2assimp
      )

      target_link_libraries(osm2assimp PUBLIC
            osm2assimpLib
            clipper s2::s2
      )

      if(MACOSX)
            target_include_directories(osm2assimp PUBLIC /usr/local/include)
      endif()

      add_executable(osm2assimpTest
            osm2assimp/test.cpp
      )
      target_link_libraries(osm2assimpTest PUBLIC
            osm2assimpLib
            poly2tri::poly2tri
            GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
            clipper
      )
endif()

option(BUILD_S2UTIL "Build s2util" ON)

if(BUILD_S2UTIL)
      add_executable(s2util
            s2util/main.cpp
            osm2assimp/eigenconversion.cpp)

      target_include_directories(s2util PRIVATE
            ext
            osm2assimp)

      target_link_libraries(s2util PUBLIC s2::s2 Eigen3::Eigen)
endif()