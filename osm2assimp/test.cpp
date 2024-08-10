#include "gtest/gtest.h"

#include "assimpwriter.h"
#include "geometry.h"
#include "glm/glm.hpp"
#include "ground.h"
#include "utils.h"
#include <filesystem>
#include <glm/ext/scalar_constants.hpp>
#include <vector>

using namespace GeoUtils;
namespace fs = std::filesystem;

TEST(Test, MeshFromLine) {
  std::vector<glm::vec2> points = {{0.0, 0.0}, {0.0, 10.0}, {10.0, 20.0}};

  try {
    Geometry::meshFromLine(points, 2.0, 0);
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

    auto tri = Triangulate(flat);

    ground.addFootPrint(flat);
  }

  for (auto &p : input) {
    p.x += 5.0;
    p.y += 5.0;
  };

  {
    auto flat = Geometry::DataFlat{input, {}};

    auto tri = Triangulate(flat);

    ground.addFootPrint(flat);
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

    auto footprint = Triangulate(flat);

    ground.addFootPrint(flat);
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

    auto result = lineIntersects2d(line0, line1, &intersection);

    EXPECT_TRUE(result);
    EXPECT_FLOAT_EQ(intersection.x, 5.0f);
  }
  {

    auto line0 = Line{glm::vec2{0.0f, 0.0f}, {0.0f, 2.0f}};
    auto line1 = Line{glm::vec2{0.0f, 2.0f}, {2.0f, 1.0f}};

    auto lineDir = glm::normalize(line1[1] - line1[0]);
    line1[0] += lineDir * glm::epsilon<float>();
    line1[1] -= lineDir * glm::epsilon<float>();

    glm::vec2 intersection;

    auto result = lineIntersects2d(line0, line1, &intersection);

    EXPECT_FALSE(result);
  }
}
TEST(Test, TriangulateConvex) {

  std::vector<glm::vec2> convex = {{0.0, 0.0}, {0.0, 1.0}, {0.5, 1.5},
                                   {1.5, 1.5}, {2.0, 1.0}, {2.0, 0.0}};

  auto footprint = Geometry::DataFlat{convex, {}};
  auto tri = Triangulate(footprint);

  footprint.writeSvg(testDir() / "TriangulateConvex.svg");

  EXPECT_EQ(footprint.mFaces.size(), 4);
}

TEST(Test, TriangulateL) {

  std::vector<glm::vec2> L = {{0.0, 0.0}, {0.0, 2.0}, {2.0, 2.0},
                              {2.0, 1.0}, {1.0, 1.0}, {1.0, 0.0}};

  auto footprint = Geometry::DataFlat{L, {}};
  auto tri = Triangulate(footprint);

  footprint.writeSvg(testDir() / "TriangulateL.svg");
}
TEST(Test, TriangulateDonut) {

  std::vector<glm::vec2> awkward = {
      {0.0, 0.0}, {0.0, 4.0}, {4.0, 4.0}, {4.0, 0.0}, {2.0, 0.0}, {2.5, 2.5},
      {3.0, 0.2}, {3.0, 3.0}, {1.0, 3.0}, {1.0, 2.5}, {2.0, 2.5}, {2.0, 1.5},
      {1.0, 1.5}, {1.0, 1.0}, {1.5, 1.0}, {1.5, 0.0}};

  auto footprint = Geometry::DataFlat{awkward, {}};
  auto tri = Triangulate(footprint);

  footprint.writeSvg(testDir() / "TriangulateAwkward.svg");
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

auto main(int argc, char **argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}