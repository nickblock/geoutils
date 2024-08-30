#include "gtest/gtest.h"

#include "assimpwriter.h"
#include "geometry.h"
#include "glm/glm.hpp"
#include "ground.h"
#include "liminalspaces.h"
#include "roadgraph.h"
#include "svg.h"
#include "utils.h"
#include <filesystem>
#include <glm/ext/scalar_constants.hpp>
#include <vector>

using namespace GeoUtils;
namespace fs = std::filesystem;

TEST(Test, MeshFromLine) {
  std::vector<glm::vec2> points = {{0.0, 0.0}, {0.0, 10.0}, {10.0, 10.0}};

  try {
    auto geometry = Geometry::meshFromLine(points, 2.0, 0);

    auto aiMesh = geometry.toMesh();
    EXPECT_EQ(aiMesh->mNumVertices, 6);

    auto footprint = geometry.getFootprint();
    SVGWriter().addPolygons(footprint).write(testDir() / "MeshFromLine.svg");
    EXPECT_EQ(footprint.mFaces.size(), 1);
    EXPECT_EQ(footprint.mFaces[0].size(), 6);

    auto tri = Triangulate(footprint).triangulate();

    SVGWriter().addPolygons(*tri).write(testDir() / "MeshFromLineTri.svg");

  } catch (std::runtime_error &err) {
    EXPECT_TRUE(false);
  }
}
TEST(Test, ExtrudeMesh) {

  std::vector<glm::vec2> vertices = {{2.0, 2.0}, {2.0, 4.0}, {3.0, 5.0},
                                     {4.0, 4.0}, {4.0, 2.0}, {6.0, 6.0},
                                     {6.0, 8.0}, {8.0, 8.0}, {8.0, 6.0}};

  Geometry::DataFlat flatData;

  flatData.mVertices = vertices;

  flatData.mFaces.resize(2);
  flatData.mFaces[0] = {0, 1, 2, 3, 4};
  flatData.mFaces[1] = {5, 6, 7, 8};

  auto data3d = flatData.extrude3DFromFlat(2.0f, 0.f);

  EXPECT_EQ(data3d.mVertices.size(), 54);
  EXPECT_EQ(data3d.mFaces.size(), 13);

  AssimpWriter writer;
  writer.addMesh(data3d.toMesh());
  EXPECT_EQ(0, writer.write(testDir() / "ExtrudeMesh.obj"));
}
TEST(Test, JunctionGeometry) {
  auto center = glm::vec2(10.f, 10.f);

  auto spokes = std::vector<glm::vec2>{
      {0.f, 10.f}, {10.f, 20.f}, {20.f, 10.f}, {10.f, 0.f}};

  auto geometry = Geometry::meshFromJunction(center, spokes, 2.0f, 0.0f);

  auto footprint = geometry.getFootprint();
  SVGWriter().addPolygons(footprint).write(testDir() / "Junction.svg");
}

TEST(Test, GroundTest) {
  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<glm::vec2> input = {
      {2.0f, 2.0f}, {2.0f, 6.0}, {6.0, 6.0}, {6.0, 2.0f}};

  {
    auto flat = Geometry::DataFlat{input, {}};

    auto tri = Triangulate(flat).triangulate();

    ground.addFootPrint(*tri, OSMFeature::HIGHWAY);
  }

  for (auto &p : input) {
    p.x += 5.0;
    p.y += 5.0;
  };

  {
    auto flat = Geometry::DataFlat{input, {}};

    auto tri = Triangulate(flat).triangulate();

    ground.addFootPrint(*tri, OSMFeature::HIGHWAY);
  }

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  ground.writeSvg(testDir() / "GroundTest.svg");

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(testDir() / "GroundTest.fbx"));
}

TEST(Test, IntersectLine) {
  {

    auto line0 = Line{glm::vec2{0.0f, 0.0f}, {20.0f, 20.0f}};
    auto line1 = Line{glm::vec2{5.0f, 0.0f}, {5.0f, 20.0f}};
    glm::vec2 intersection;

    auto result = lineIntersects2d(line0, line1);

    EXPECT_TRUE(std::get<bool>(result));
    EXPECT_FLOAT_EQ(std::get<glm::vec2>(result).x, 5.0f);
  }
  {

    auto line0 = Line{glm::vec2{0.0f, 0.0f}, {0.0f, 2.0f}};
    auto line1 = Line{glm::vec2{0.0f, 2.0f}, {2.0f, 1.0f}};

    auto lineDir = glm::normalize(line1[1] - line1[0]);
    line1[0] += lineDir * glm::epsilon<float>();
    line1[1] -= lineDir * glm::epsilon<float>();
    auto result = lineIntersects2d(line0, line1);

    EXPECT_FALSE(std::get<bool>(result));
  }
}

TEST(Test, ReflexPoint) {

  {
    // right turn

    auto a = glm::vec2(0.f, 0.f);
    auto b = glm::vec2(0.f, 1.f);
    auto c = glm::vec2(1.f, 1.f);

    auto reflex = Triangulate::reflexPoint(a, b, c);
    EXPECT_LT(reflex, 0.0f);
  }
  {
    // left turn

    auto a = glm::vec2(0.f, 0.f);
    auto b = glm::vec2(0.f, 1.f);
    auto c = glm::vec2(-1.f, 1.f);

    auto reflex = Triangulate::reflexPoint(a, b, c);
    EXPECT_GT(reflex, 0.0f);
  }
}

TEST(Test, TriangulateWindingOrder)

{
  std::vector<glm::vec2> cw = {{0.0, 1.0}, {1.0, 1.0}, {2.0, 1.0},
                               {2.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}};
  {

    auto footprint = Geometry::DataFlat{cw, {}};
    EXPECT_TRUE(Triangulate(footprint).checkWindingOrder());
  }

  {

    std::reverse(cw.begin(), cw.end());

    auto footprint = Geometry::DataFlat{cw, {}};
    EXPECT_FALSE(Triangulate(footprint).checkWindingOrder());
  }
}

TEST(Test, TriangulateConvex) {

  std::vector<glm::vec2> convex = {{0.0, 0.0}, {0.0, 1.0}, {0.5, 1.5},
                                   {1.5, 1.5}, {2.0, 1.0}, {2.0, 0.0}};

  // test with CCW input (should get reversed by triangulate)
  std::reverse(convex.begin(), convex.end());

  auto footprint = Geometry::DataFlat{convex, {}};
  auto tri = Triangulate(footprint).triangulate();

  SVGWriter().addPolygons(*tri).write(testDir() / "TriangulateConvex.svg");

  EXPECT_EQ(tri->mFaces.size(), 4);
}

TEST(Test, TriangulateL) {

  std::vector<glm::vec2> L = {{0.0, 0.0}, {0.0, 2.0}, {2.0, 2.0},
                              {2.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}};

  auto footprint = Geometry::DataFlat{L, {}};
  auto tri = Triangulate(footprint).triangulate();

  SVGWriter().addPolygons(*tri).write(testDir() / "TriangulateL.svg");
}
TEST(Test, TriangulateDonut) {

  std::vector<glm::vec2> awkward = {
      {0.0, 0.0}, {0.0, 4.0}, {4.0, 4.0}, {4.0, 0.0}, {2.0, 0.0}, {2.5, 2.5},
      {3.0, 0.2}, {3.0, 3.0}, {1.0, 3.0}, {1.0, 2.5}, {2.0, 2.5}, {2.0, 1.5},
      {1.0, 1.5}, {1.0, 1.0}, {1.5, 1.0}, {1.5, 0.0}};

  auto footprint = Geometry::DataFlat{awkward, {}};
  auto tri = Triangulate(footprint).triangulate();

  SVGWriter().addPolygons(*tri).write(testDir() / "TriangulateAwkward.svg");
}

TEST(Test, PointOnLine) {
  {

    auto line = Line{glm::vec2{0.0f, 0.0f}, glm::vec2{1.0f, 2.0f}};

    auto point = glm::vec2(0.5, 1.0);

    bool on = pointOnLine(line, point);
    EXPECT_TRUE(on);

    point = glm::vec2(0.5, 1.5);
    on = pointOnLine(line, point);
    EXPECT_FALSE(on);
  }
  {

    auto line = Line{glm::vec2{0.0f, 4.0f}, glm::vec2{4.0f, 0.0f}};
    auto point = glm::vec2(3.0, 1.0);

    bool on = pointOnLine(line, point);
    EXPECT_TRUE(on);
  }
}

TEST(Test, SpaceNumSegments) {

  PointCache cache;
  Space spline(cache);

  spline.append({0.0, 0.0});
  spline.append({0.0, 1.0});
  spline.append({1.0, 1.0});
  spline.append({1.0, 0.0});

  auto line0 = spline.segment(3);
  EXPECT_EQ(line0[0], spline.vertices()[3]);
  EXPECT_EQ(line0[1], spline.vertices()[0]);

  auto line1 = spline.segment(4);
  EXPECT_EQ(line1[0], spline.vertices()[0]);
  EXPECT_EQ(line1[1], spline.vertices()[1]);

  auto line2 = spline.segment(7);
  EXPECT_EQ(line2[0], spline.vertices()[3]);
  EXPECT_EQ(line2[1], spline.vertices()[0]);
}

std::vector<glm::vec2> makePointList(const glm::vec2 &begin,
                                     const glm::vec2 &end, int num) {
  std::vector<glm::vec2> result;
  auto div = (end - begin);
  div *= 1.f / (num - 1);

  for (int i = 0; i < num; i++) {
    result.push_back(begin + (div * (float)i));
  }
  return result;
}

auto makeRoad = [](const Line &line, int numPoints, float width,
                   LiminalSpaces &liminalSpaces) {
  auto road = Geometry::meshFromLine(makePointList(line[0], line[1], numPoints),
                                     width, 0.0f);
  auto tri = Triangulate(road.getFootprint()).triangulate();
  liminalSpaces.addIslands(*tri);
};
TEST(Test, LiminalSpacesGrid) {

  BBox box;
  box.add({0.0, 0.0, 0.0});
  box.add({10.0, 10.0, 0.0});
  auto liminalSpaces = LiminalSpaces(box);

  auto width = 2.0f;
  int numPoints = 2;

  // basic grid
  makeRoad(Line{glm::vec2{0.0f, 2.0f}, glm::vec2{10.0f, 2.0f}}, numPoints,
           width, liminalSpaces);
  makeRoad(Line{glm::vec2{0.0f, 8.0f}, glm::vec2{12.0f, 8.0f}}, numPoints,
           width, liminalSpaces);
  makeRoad(Line{glm::vec2{8.0f, 0.0f}, glm::vec2{8.0f, 10.0f}}, numPoints,
           width, liminalSpaces);
  makeRoad(Line{glm::vec2{2.0f, 0.0f}, glm::vec2{2.0f, 10.0f}}, numPoints,
           width, liminalSpaces);

  auto data = liminalSpaces.getInternalSpaces();

  liminalSpaces.writeSvg(testDir() / "road_grid.svg");

  EXPECT_EQ(data.mFaces.size(), 9);
}

TEST(Test, LiminalSpacesOverlap) {

  BBox box;
  box.add({0.0, 0.0, 0.0});
  box.add({15.0, 15.0, 0.0});
  auto liminalSpaces = LiminalSpaces(box);

  auto width = 2.0f;
  int numPoints = 2;

  makeRoad(Line{glm::vec2{5.0, 3.0}, glm::vec2{15.0, 3.0}}, numPoints, width,
           liminalSpaces);
  makeRoad(Line{glm::vec2{5.0, 0.0}, glm::vec2{5.0, 10.0}}, numPoints, width,
           liminalSpaces);
  makeRoad(Line{glm::vec2{10.0, 0.0}, glm::vec2{10.0, 10.0}}, numPoints, width,
           liminalSpaces);
  makeRoad(Line{glm::vec2{15.0, 0.0}, glm::vec2{15.0, 10.0}}, numPoints, width,
           liminalSpaces);
  makeRoad(Line{glm::vec2{5.0, 10.0}, glm::vec2{15.0, 10.0}}, numPoints, width,
           liminalSpaces);

  auto data = liminalSpaces.getInternalSpaces();

  liminalSpaces.writeSvg(testDir() / "road_overlap.svg");

  EXPECT_EQ(data.mFaces.size(), 5);
}

TEST(Test, LiminalSpacesLoop) {

  BBox box;
  box.add({0.0, 0.0, 0.0});
  box.add({15.0, 15.0, 0.0});
  auto liminalSpaces = LiminalSpaces(box);

  auto width = 2.0f;
  int numPoints = 2;

  std::vector<glm::vec2> points = {
      {5.f, 5.f}, {5.f, 10.f}, {10.f, 10.f}, {10.f, 5.f}, {5.f, 5.f}};

  auto road = Geometry::meshFromLine(points, width, 0.0f);
  auto tri = Triangulate(road.getFootprint()).triangulate();
  liminalSpaces.addIslands(*tri);

  auto data = liminalSpaces.getInternalSpaces();

  liminalSpaces.writeSvg(testDir() / "road_loop.svg");

  EXPECT_EQ(data.mFaces.size(), 1);
}

TEST(Test, RoadGraph) {

  auto roadGraph = RoadGraph<size_t>();

  std::vector<size_t> road0 = {0, 1, 2, 3, 4, 5};

  std::vector<size_t> road1 = {5, 6, 7};
  std::vector<size_t> road2 = {3, 8, 9};

  roadGraph.addRoad(road0);
  roadGraph.addRoad(road1);
  roadGraph.addRoad(road2);

  roadGraph.graph();

  EXPECT_EQ(roadGraph.numRoads(), 3);
  EXPECT_EQ(roadGraph.numJunctions(), 1);

  auto junction = roadGraph.getJunctions(0);

  EXPECT_EQ(junction->center, 3);
}

auto main(int argc, char **argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}