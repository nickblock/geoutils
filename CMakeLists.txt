cmake_minimum_required(VERSION 2.8)

# Name of the project (will be the name of the plugin)
project(geoutils)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1 -DGLM_FORCE_RADIANS=1 -std=c++14")


if (CMAKE_BUILD_TYPE MATCHES "Debug")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2")
endif()

option(BUILD_OSMS2SPLIT "Build osms2split" ON)

if(BUILD_OSMS2SPLIT)

      add_library(s2 STATIC
                  ext/s2geometry/src/s2/base/stringprintf.cc
                  ext/s2geometry/src/s2/base/strtoint.cc
                  ext/s2geometry/src/s2/encoded_s2cell_id_vector.cc
                  ext/s2geometry/src/s2/encoded_s2point_vector.cc
                  ext/s2geometry/src/s2/encoded_s2shape_index.cc
                  ext/s2geometry/src/s2/encoded_string_vector.cc
                  ext/s2geometry/src/s2/id_set_lexicon.cc
                  ext/s2geometry/src/s2/mutable_s2shape_index.cc
                  ext/s2geometry/src/s2/r2rect.cc
                  ext/s2geometry/src/s2/s1angle.cc
                  ext/s2geometry/src/s2/s1chord_angle.cc
                  ext/s2geometry/src/s2/s1interval.cc
                  ext/s2geometry/src/s2/s2boolean_operation.cc
                  ext/s2geometry/src/s2/s2builder.cc
                  ext/s2geometry/src/s2/s2builder_graph.cc
                  ext/s2geometry/src/s2/s2builderutil_closed_set_normalizer.cc
                  ext/s2geometry/src/s2/s2builderutil_find_polygon_degeneracies.cc
                  ext/s2geometry/src/s2/s2builderutil_lax_polygon_layer.cc
                  ext/s2geometry/src/s2/s2builderutil_s2point_vector_layer.cc
                  ext/s2geometry/src/s2/s2builderutil_s2polygon_layer.cc
                  ext/s2geometry/src/s2/s2builderutil_s2polyline_layer.cc
                  ext/s2geometry/src/s2/s2builderutil_s2polyline_vector_layer.cc
                  ext/s2geometry/src/s2/s2builderutil_snap_functions.cc
                  ext/s2geometry/src/s2/s2cap.cc
                  ext/s2geometry/src/s2/s2cell.cc
                  ext/s2geometry/src/s2/s2cell_id.cc
                  ext/s2geometry/src/s2/s2cell_index.cc
                  ext/s2geometry/src/s2/s2cell_union.cc
                  ext/s2geometry/src/s2/s2centroids.cc
                  ext/s2geometry/src/s2/s2closest_cell_query.cc
                  ext/s2geometry/src/s2/s2closest_edge_query.cc
                  ext/s2geometry/src/s2/s2closest_point_query.cc
                  ext/s2geometry/src/s2/s2contains_vertex_query.cc
                  ext/s2geometry/src/s2/s2convex_hull_query.cc
                  ext/s2geometry/src/s2/s2coords.cc
                  ext/s2geometry/src/s2/s2crossing_edge_query.cc
                  ext/s2geometry/src/s2/s2debug.cc
                  ext/s2geometry/src/s2/s2earth.cc
                  ext/s2geometry/src/s2/s2edge_clipping.cc
                  ext/s2geometry/src/s2/s2edge_crosser.cc
                  ext/s2geometry/src/s2/s2edge_crossings.cc
                  ext/s2geometry/src/s2/s2edge_distances.cc
                  ext/s2geometry/src/s2/s2edge_tessellator.cc
                  ext/s2geometry/src/s2/s2error.cc
                  ext/s2geometry/src/s2/s2furthest_edge_query.cc
                  ext/s2geometry/src/s2/s2latlng.cc
                  ext/s2geometry/src/s2/s2latlng_rect.cc
                  ext/s2geometry/src/s2/s2latlng_rect_bounder.cc
                  ext/s2geometry/src/s2/s2lax_loop_shape.cc
                  ext/s2geometry/src/s2/s2lax_polygon_shape.cc
                  ext/s2geometry/src/s2/s2lax_polyline_shape.cc
                  ext/s2geometry/src/s2/s2loop.cc
                  ext/s2geometry/src/s2/s2loop_measures.cc
                  ext/s2geometry/src/s2/s2measures.cc
                  ext/s2geometry/src/s2/s2metrics.cc
                  ext/s2geometry/src/s2/s2max_distance_targets.cc
                  ext/s2geometry/src/s2/s2min_distance_targets.cc
                  ext/s2geometry/src/s2/s2padded_cell.cc
                  ext/s2geometry/src/s2/s2point_compression.cc
                  ext/s2geometry/src/s2/s2point_region.cc
                  ext/s2geometry/src/s2/s2pointutil.cc
                  ext/s2geometry/src/s2/s2polygon.cc
                  ext/s2geometry/src/s2/s2polyline.cc
                  ext/s2geometry/src/s2/s2polyline_alignment.cc
                  ext/s2geometry/src/s2/s2polyline_measures.cc
                  ext/s2geometry/src/s2/s2polyline_simplifier.cc
                  ext/s2geometry/src/s2/s2predicates.cc
                  ext/s2geometry/src/s2/s2projections.cc
                  ext/s2geometry/src/s2/s2r2rect.cc
                  ext/s2geometry/src/s2/s2region.cc
                  ext/s2geometry/src/s2/s2region_term_indexer.cc
                  ext/s2geometry/src/s2/s2region_coverer.cc
                  ext/s2geometry/src/s2/s2region_intersection.cc
                  ext/s2geometry/src/s2/s2region_union.cc
                  ext/s2geometry/src/s2/s2shape_index.cc
                  ext/s2geometry/src/s2/s2shape_index_buffered_region.cc
                  ext/s2geometry/src/s2/s2shape_index_measures.cc
                  ext/s2geometry/src/s2/s2shape_measures.cc
                  ext/s2geometry/src/s2/s2shapeutil_build_polygon_boundaries.cc
                  ext/s2geometry/src/s2/s2shapeutil_coding.cc
                  ext/s2geometry/src/s2/s2shapeutil_contains_brute_force.cc
                  ext/s2geometry/src/s2/s2shapeutil_edge_iterator.cc
                  ext/s2geometry/src/s2/s2shapeutil_get_reference_point.cc
                  ext/s2geometry/src/s2/s2shapeutil_range_iterator.cc
                  ext/s2geometry/src/s2/s2shapeutil_visit_crossing_edge_pairs.cc
                  ext/s2geometry/src/s2/s2text_format.cc
                  ext/s2geometry/src/s2/s2wedge_relations.cc
                  ext/s2geometry/src/s2/strings/ostringstream.cc
                  ext/s2geometry/src/s2/strings/serialize.cc
                  ext/s2geometry/src/s2/third_party/absl/base/dynamic_annotations.cc
                  ext/s2geometry/src/s2/third_party/absl/base/internal/raw_logging.cc
                  ext/s2geometry/src/s2/third_party/absl/base/internal/throw_delegate.cc
                  ext/s2geometry/src/s2/third_party/absl/numeric/int128.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/ascii.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/match.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/numbers.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/str_cat.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/str_split.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/string_view.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/strip.cc
                  ext/s2geometry/src/s2/third_party/absl/strings/internal/memutil.cc
                  ext/s2geometry/src/s2/util/bits/bit-interleave.cc
                  ext/s2geometry/src/s2/util/bits/bits.cc
                  ext/s2geometry/src/s2/util/coding/coder.cc
                  ext/s2geometry/src/s2/util/coding/varint.cc
                  ext/s2geometry/src/s2/util/math/exactfloat/exactfloat.cc
                  ext/s2geometry/src/s2/util/math/mathutil.cc
                  ext/s2geometry/src/s2/util/units/length-units.cc)

      target_include_directories(s2 PUBLIC ext/s2geometry/src)
      target_compile_options(s2 PRIVATE -Wno-attributes)

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
                  s2 
                  ${CMAKE_CURRENT_LIST_DIR}/ext/openssl/mac/libopenssl.a
                  -lz -lbz2 -lexpat -lpthread)
      else()
            target_link_libraries(osms2split PUBLIC s2 ssl crypto bz2 z expat pthread)
      endif()

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
            ext/rapidjson/include 
            ext/pnghpp 
            ext )

      target_include_directories(osmsplitlib PUBLIC ${OSMSPLIT_INCLUDES} )
      target_include_directories(osmsplit PUBLIC ${OSMSPLIT_INCLUDES})

      if(MACOSX)
            target_include_directories(osmsplit PUBLIC /usr/local/include )
            link_directories(/usr/local/lib)
            target_link_libraries(osmsplit PUBLIC osmsplitlib pthread bz2 z expat png)
      else()
            target_link_libraries(osmsplit PUBLIC osmsplitlib pthread stdc++fs bz2 z expat png)
      endif()
endif()

option(BUILD_OSM2ASSIMP "Build osm2assimp" ON)
option(BUILD_GTEST "Build GoogleTest" ON)


if(BUILD_OSM2ASSIMP)

      add_subdirectory(ext/assimp)

      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPACKAGE_VERSION=\"1\" -fcommon")

      add_library(clipper STATIC
            ext/clipper/clipper.cpp)
      
      target_include_directories(clipper PRIVATE
            ext/clipper )

      add_library(osm2assimpLib STATIC 
            osm2assimp/assimpconstruct.cpp
            osm2assimp/convertlatlng.cpp
            osm2assimp/eigenconversion.cpp
            osm2assimp/osmdata.cpp
            osm2assimp/osmfeature.cpp
            osm2assimp/geomconvert.cpp
            osm2assimp/viewfilter.cpp
            ext/common.cpp
      )


      target_include_directories(osm2assimpLib PUBLIC 
            osm2assimp
            ext/libosmium/include 
            ext/protozero/include
            ${CMAKE_CURRENT_BINARY_DIR}/ext/assimp/include
            ext/assimp/include
            ext/clipper
            ext/eigen 
            ext/rapidjson/include 
            ext/glm 
            ext/s2geometry/src
            ext 
            s2util 
      )

      add_executable(osm2assimp
            osm2assimp/main.cpp
      )

      target_include_directories(osm2assimp PUBLIC 
            osm2assimp 
      )
      
      if(MACOSX)
      target_link_libraries(osm2assimp PUBLIC assimp
      osm2assimpLib
      bz2 z expat pthread clipper s2)
      target_include_directories(osm2assimp PUBLIC /usr/local/include )
      else()
      target_link_libraries(osm2assimp PUBLIC osm2assimpLib assimp s2 ssl crypto bz2 z expat pthread clipper )
      endif()

      if(BUILD_GTEST)
            message("Building GTest from Geoutils")
            add_subdirectory(ext/googletest)
      endif()

      add_executable(osm2assimpTest 
            osm2assimp/test.cpp
      )
      target_include_directories(osm2assimpTest PUBLIC
            ext/googletest/googletest/include
      )
      target_link_libraries(osm2assimpTest PUBLIC 
            osm2assimpLib
            gtest_main
            clipper
      )

endif()

option(BUILD_S2UTIL "Build s2util" ON)

if(BUILD_S2UTIL)

      add_executable(s2util 
            s2util/main.cpp)

      target_include_directories(s2util PRIVATE ext)
      target_include_directories(s2util PRIVATE ext/s2geometry/src)
      if(MACOSX)
            target_link_libraries(s2util PUBLIC s2 bz2 pthread)
      else()
            target_link_libraries(s2util PUBLIC s2 ssl crypto bz2 pthread)
      endif()
endif()