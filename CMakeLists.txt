cmake_minimum_required(VERSION 2.8)

# Name of the project (will be the name of the plugin)
project(osmsplits2)

add_subdirectory(ext/assimp)

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

target_link_libraries(osms2split PUBLIC s2 ssl crypto bz2 z expat pthread)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRAPIDJSON_HAS_STDSTRING=1")

add_executable(osmsplit
      osmsplit/main.cpp 
      osmsplit/osmsplitconfig.cpp 
      osmsplit/osmsplitwriter.cpp)

target_include_directories(osmsplit PUBLIC ext/libosmium/include )
target_include_directories(osmsplit PUBLIC ext/protozero/include)
target_include_directories(osmsplit PUBLIC ext/rapidjson/include )
target_include_directories(osmsplit PUBLIC ext/pnghpp )
target_include_directories(osmsplit PUBLIC ext )

target_link_libraries(osmsplit PUBLIC pthread stdc++fs bz2 z expat png)


add_executable(osm2assimp
      osm2assimp/assimp_construct.cpp
      osm2assimp/centerearthfixedconvert.cpp
      osm2assimp/eigenconversion.cpp
      osm2assimp/main.cpp
      osm2assimp/osmdata.cpp
      osm2assimp/osmfeature.cpp
      osm2assimp/testtrig.cpp)

target_include_directories(osm2assimp PUBLIC osm2assimp )
target_include_directories(osm2assimp PUBLIC ext/libosmium/include )
target_include_directories(osm2assimp PUBLIC ext/protozero/include)
target_include_directories(osm2assimp PUBLIC ext/assimp/include)
target_include_directories(osm2assimp PUBLIC ext/gfxpoly)
target_include_directories(osm2assimp PUBLIC ext/eigen )
target_include_directories(osm2assimp PUBLIC ext/rapidjson/include )
target_include_directories(osm2assimp PUBLIC ext )
target_link_libraries(osm2assimp PUBLIC assimp)
