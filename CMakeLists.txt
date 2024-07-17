cmake_minimum_required(VERSION 3.22.6)

# Name of the project (will be the name of the plugin)
project(geoutils)


find_package(GTest REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(assimp REQUIRED)
find_package(poly2tri REQUIRED)
find_package(glm REQUIRED)
find_package(s2 REQUIRED)


if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(MACOSX TRUE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1 -DGLM_FORCE_RADIANS=1 -std=c++14")


if (CMAKE_BUILD_TYPE MATCHES "Debug")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")
endif()

option(BUILD_OSMSPLIT "Build osmsplit" ON)

if(BUILD_OSMSPLIT)

      add_library(osmsplitlib STATIC
            osmsplit/osmsplitconfig.cpp 
            osmsplit/osmsplitwriter.cpp )

      add_executable(osmsplit
            osmsplit/main.cpp )

      set(OSMSPLIT_INCLUDES
            ext/libosmium/include 
            ext/protozero/include
            ext/pnghpp 
            ext )

      target_include_directories(osmsplitlib PUBLIC ${OSMSPLIT_INCLUDES} )
      target_include_directories(osmsplit PUBLIC ${OSMSPLIT_INCLUDES})

      target_link_libraries(osmsplitlib PUBLIC rapidjson)

      if(MACOSX)
            target_include_directories(osmsplit PUBLIC /usr/local/include )
            link_directories(/usr/local/lib)
            target_link_libraries(osmsplit PUBLIC osmsplitlib pthread bz2 z expat png rapidjson)
      else()
            target_link_libraries(osmsplit PUBLIC osmsplitlib pthread stdc++fs bz2 z expat png rapidjson)
      endif()
endif()


option(BUILD_OSMS2SPLIT "Build osms2split" ON)

if(BUILD_OSMS2SPLIT)

add_executable(osms2split
        osms2split/src/main.cpp
        osms2split/src/s2splitter.cpp)

      target_compile_options(osms2split PRIVATE -Wno-attributes)
      target_include_directories(osms2split PUBLIC ext/protozero/include)
      target_include_directories(osms2split PUBLIC ext/libosmium/include)
      target_include_directories(osms2split PUBLIC ext/s2geometry/src)
      target_include_directories(osms2split PUBLIC ext)

      if(MACOSX)
            target_include_directories(s2 PUBLIC ext/openssl )
            link_directories(/usr/local/opt/bzip2/lib ${CMAKE_CURRENT_LIST_DIR}/ext/openssl/mac /usr/local/lib)
            target_link_libraries(osms2split PUBLIC 
                  s2::s2
                  ${CMAKE_CURRENT_LIST_DIR}/ext/openssl/mac/libopenssl.a
                  -lz -lbz2 -lexpat -lpthread)
      else()
            target_link_libraries(osms2split PUBLIC s2::s2 ssl crypto bz2 z expat pthread)
      endif()

endif()

option(BUILD_OSM2ASSIMP "Build osm2assimp" ON)

if(BUILD_OSM2ASSIMP)

      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPACKAGE_VERSION=\"1\" -fcommon")

      add_library(clipper STATIC
            ext/clipper/clipper.cpp)
      
      target_include_directories(clipper PRIVATE
            ext/clipper )

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
            osm2assimp
            ext/libosmium/include 
            ext/protozero/include
            ext/clipper
            ext/eigen
            ext 
            s2util 
      )
      target_link_libraries(osm2assimpLib PUBLIC assimp::assimp poly2tri::poly2tri glm::glm s2::s2)

      add_executable(osm2assimp
            osm2assimp/main.cpp
      )

      target_include_directories(osm2assimp PUBLIC 
            osm2assimp 
      )
      
      if(MACOSX)
      target_link_libraries(osm2assimp PUBLIC assimp::assimp
      osm2assimpLib
      bz2 z expat pthread clipper s2::s2)
      target_include_directories(osm2assimp PUBLIC /usr/local/include )
      else()
      target_link_libraries(osm2assimp PUBLIC osm2assimpLib assimp::assimp s2 ssl crypto bz2 z expat pthread clipper )
      endif()


      add_executable(osm2assimpTest 
            osm2assimp/test.cpp
      )
      target_link_libraries(osm2assimpTest PUBLIC 
            osm2assimpLib
            poly2tri clipper
            gtest::gtest
            clipper
            assimp::assimp
            s2::s2
            bz2
            expat
      )

endif()

option(BUILD_S2UTIL "Build s2util" ON)

if(BUILD_S2UTIL)

      add_executable(s2util 
            s2util/main.cpp
            osm2assimp/eigenconversion.cpp)

      target_include_directories(s2util PRIVATE 
            ext
            osm2assimp
            ext/eigen)
            
      if(MACOSX)
            target_link_libraries(s2util PUBLIC s2::s2 bz2 pthread)
      else()
            target_link_libraries(s2util PUBLIC s2::s2 ssl crypto bz2 pthread)
      endif()
endif()