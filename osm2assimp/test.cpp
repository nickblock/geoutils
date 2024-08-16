#include "gtest/gtest.h"

#include "assimpwriter.h"
#include "geometry.h"
#include "glm/glm.hpp"
#include "ground.h"
#include "roadnetwork.h"
#include "svg.h"
#include "utils.h"
#include <filesystem>
#include <glm/ext/scalar_constants.hpp>
#include <vector>

using namespace GeoUtils;
namespace fs = std::filesystem;

TEST(Test, MeshFromLine) {
  std::vector<glm::vec2> points = {{0.0, 0.0}, {0.0, 10.0}, {10.0, 20.0}};

  try {
    auto geometry = Geometry::meshFromLine(points, 2.0, 0);

    auto aiMesh = geometry.simpleMesh();
    EXPECT_EQ(aiMesh->mNumVertices, 6);

    auto footprint = geometry.getFootprint();
    SVGWriter().addPolygons(footprint).write(testDir() / "MeshFromLine.svg");
    EXPECT_EQ(footprint.mFaces.size(), 1);
    EXPECT_EQ(footprint.mFaces[0].size(), 6);

    auto tri = Triangulate(footprint).getData();

    SVGWriter().addPolygons(*tri).write(testDir() / "MeshFromLineTri.svg");

  } catch (std::runtime_error &err) {
    EXPECT_TRUE(false);
  }
}

TEST(Test, GroundTest) {
  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<glm::vec2> input = {
      {2.0f, 2.0f}, {2.0f, 6.0}, {6.0, 6.0}, {6.0, 2.0f}};

  {
    auto flat = Geometry::DataFlat{input, {}};

    auto tri = Triangulate(flat).getData();

    ground.addFootPrint(*tri, Ground::Building);
  }

  for (auto &p : input) {
    p.x += 5.0;
    p.y += 5.0;
  };

  {
    auto flat = Geometry::DataFlat{input, {}};

    auto tri = Triangulate(flat).getData();

    ground.addFootPrint(*tri, Ground::Building);
  }

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  ground.writeSvg(testDir() / "GroundTest.svg");

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(testDir() / "GroundTest.fbx"));
}

TEST(Test, GroundDonut) {

  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  {

    std::vector<glm::vec2> verts = {
        {2.0, 2.0}, {2.0, 6.0}, {6.0, 6.0}, {6.0, 2.0}, {4.0, 2.0}, {4.0, 3.0},
        {5.0, 3.0}, {5.0, 5.0}, {3.0, 5.0}, {3.0, 3.0}, {3.5, 3.0}, {3.5, 2.0}};

    auto flat = Geometry::DataFlat(verts, {});

    auto tri = Triangulate(flat).getData();

    ground.addFootPrint(*tri, Ground::Road);
  }

  auto groundMesh = ground.getMesh();

  EXPECT_NE(groundMesh, nullptr);

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(testDir() / "GroundDonut.fbx"));
}

TEST(Test, IntersectLine) {
  {

    auto line0 = Line{glm::vec2{0.0f, 0.0f}, {20.0f, 20.0f}};
    auto line1 = Line{glm::vec2{5.0f, 0.0f}, {5.0f, 20.0f}};
    glm::vec2 intersection;

    auto result = lineIntersects2d(line0, line1);

    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ((*result).x, 5.0f);
  }
  {

    auto line0 = Line{glm::vec2{0.0f, 0.0f}, {0.0f, 2.0f}};
    auto line1 = Line{glm::vec2{0.0f, 2.0f}, {2.0f, 1.0f}};

    auto lineDir = glm::normalize(line1[1] - line1[0]);
    line1[0] += lineDir * glm::epsilon<float>();
    line1[1] -= lineDir * glm::epsilon<float>();
    auto result = lineIntersects2d(line0, line1);

    EXPECT_FALSE(result);
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
    EXPECT_TRUE(Triangulate(footprint).checkWindingOrder());
  }
}

TEST(Test, TriangulateConvex) {

  std::vector<glm::vec2> convex = {{0.0, 0.0}, {0.0, 1.0}, {0.5, 1.5},
                                   {1.5, 1.5}, {2.0, 1.0}, {2.0, 0.0}};

  // test with CCW input (should get reversed by triangulate)
  std::reverse(convex.begin(), convex.end());

  auto footprint = Geometry::DataFlat{convex, {}};
  auto tri = Triangulate(footprint).getData();

  SVGWriter().addPolygons(*tri).write(testDir() / "TriangulateConvex.svg");

  EXPECT_EQ(tri->mFaces.size(), 4);
}

TEST(Test, TriangulateL) {

  std::vector<glm::vec2> L = {{0.0, 0.0}, {0.0, 2.0}, {2.0, 2.0},
                              {2.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}};

  auto footprint = Geometry::DataFlat{L, {}};
  auto tri = Triangulate(footprint).getData();

  SVGWriter().addPolygons(*tri).write(testDir() / "TriangulateL.svg");
}
TEST(Test, TriangulateDonut) {

  std::vector<glm::vec2> awkward = {
      {0.0, 0.0}, {0.0, 4.0}, {4.0, 4.0}, {4.0, 0.0}, {2.0, 0.0}, {2.5, 2.5},
      {3.0, 0.2}, {3.0, 3.0}, {1.0, 3.0}, {1.0, 2.5}, {2.0, 2.5}, {2.0, 1.5},
      {1.0, 1.5}, {1.0, 1.0}, {1.5, 1.0}, {1.5, 0.0}};

  auto footprint = Geometry::DataFlat{awkward, {}};
  auto tri = Triangulate(footprint).getData();

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

TEST(Test, RoadNetwork) {

  BBox box;
  box.add({0.0, 0.0, 0.0});
  box.add({15.0, 15.0, 0.0});
  auto roadNetwork = RoadNetwork(box);

  auto width = 2.0f;
  int numPoints = 2;

  auto makeRoad = [numPoints, width, &roadNetwork](const glm::vec2 &a,
                                                   const glm::vec2 &b) {
    auto road = Geometry::meshFromLine(makePointList(a, b, numPoints), width);
    auto tri = Triangulate(road.getFootprint()).getData();
    roadNetwork.addRoad(*tri);
  };

  // basic grid
  // makeRoad({0.0f, 2.0f}, {10.0f, 2.0f});
  // makeRoad({0.0f, 8.0f}, {12.0f, 8.0f});
  // makeRoad({8.0f, 0.0f}, {8.0f, 10.0f});
  // makeRoad({2.0f, 0.0f}, {2.0f, 10.0f});

  makeRoad({5.0, 3.0}, {15.0, 3.0});
  makeRoad({5.0, 0.0}, {5.0, 10.0});
  makeRoad({10.0, 0.0}, {10.0, 10.0});
  makeRoad({15.0, 0.0}, {15.0, 10.0});
  makeRoad({5.0, 10.0}, {15.0, 10.0});

  // given a simple grid of four interesecting roads we would like to get the
  // internal rectangle contained

  auto internalSpace = roadNetwork.getInternalSpaces();

  // SVGWriter().addPolygons(internalSpace).write(testDir() /
  // "RoadNetwork.svg");
}

auto main(int argc, char **argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}