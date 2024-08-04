#include "gtest/gtest.h"

#include "assimpwriter.h"
#include "clipper.hpp"
#include "geometry.h"
#include "glm/glm.hpp"
#include "ground.h"
#include "triangulate.h"
#include "utils.h"
#include <filesystem>

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

TEST(Test, ClipperTest) {
  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<double> clip0 = {
      2.0f, 2.0f, 2.0f, 6.0, 6.0, 6.0, 6.0, 2.0f, 2.0f, 2.0f,
  };

  ground.addFootPrint(clip0, 0);

  std::vector<double> clip1;

  for (auto &p : clip0) {
    clip1.push_back(p + 5.0);
  };

  ground.addFootPrint(clip1, 0);
  ground.writeSvg(testDir() / "ClipperTest.svg", 100.0);

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(testDir() / "ClipperTest.fbx"));
}

TEST(Test, GroundDonut) {

  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<double> donut = {2.0, 2.0, 2.0, 6.0, 6.0, 6.0, 6.0, 2.0,
                               4.0, 2.0, 4.0, 3.0, 5.0, 3.0, 5.0, 5.0,
                               3.0, 5.0, 3.0, 3.0, 3.5, 3.0, 3.5, 2.0};

  ground.addFootPrint(donut, 0);

  ground.writeSvg(testDir() / "GroundDonut.svg", 100.0);

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(testDir() / "GroundDonut.fbx"));
}
TEST(Test, ClipperLibIntersect) {

  std::vector<glm::vec2> clip0 = {
      {2.0f, 2.0f}, {2.0f, 6.0}, {6.0, 6.0}, {6.0, 2.0f}};
  std::vector<glm::vec2> clip1;

  for (auto &p : clip0) {
    clip1.push_back(p + glm::vec2(2.0, 2.0));
  };

  auto result = intersectPolygons(clip0, clip1);

  EXPECT_EQ(1, result.size());
  EXPECT_EQ(true, polyOrientation(result[0]));

  for (auto &p : clip1) {
    p += glm::vec2(5.0, 5.0);
  };

  result = intersectPolygons(clip0, clip1);

  EXPECT_EQ(2, result.size());
  EXPECT_EQ(true, polyOrientation(result[0]));
  EXPECT_EQ(true, polyOrientation(result[0]));
}

TEST(Test, ClipperSubtractPoly) {

  std::vector<glm::vec2> background = {
      {0.0, 0.0}, {0.0, 10.0}, {10.0, 10.0}, {10.0, 0.0}};

  std::vector<glm::vec2> donut = {
      {2.0, 2.0}, {2.0, 6.0}, {6.0, 6.0}, {6.0, 2.0}, {4.0, 2.0}, {4.0, 3.0},
      {5.0, 3.0}, {5.0, 5.0}, {3.0, 5.0}, {3.0, 3.0}, {4.0, 3.0}, {4.0, 2.0}};

  auto result = intersectPolygons(background, donut, 0 /*intersection*/);

  int idx = 0;
  for (auto &v : result) {
    writeSvg(v, 100,
             testDir() / std::format("ClipperSubtractPoly{}.svg", idx++));
  }
}

TEST(Test, IntersectLine) {
  auto line0 = Line{glm::vec2{0.0f, 0.0f}, {20.0f, 20.0f}};
  auto line1 = Line{glm::vec2{5.0f, 0.0f}, {5.0f, 20.0f}};
  glm::vec2 intersection;

  auto result = lineIntersects2d(line0, line1, &intersection);

  EXPECT_TRUE(result);
  EXPECT_FLOAT_EQ(intersection.x, 5.0f);
}
TEST(Test, TriangulateConvex) {

  std::vector<glm::vec2> convex = {{0.0, 0.0}, {0.0, 1.0}, {0.5, 1.5},
                                   {1.5, 1.5}, {2.0, 1.0}, {2.0, 0.0}};

  auto faceList = Geometry::triangulate(convex);

  Geometry::writeSvg(faceList, convex, testDir() / "TriangulateConvex.svg");

  Triangulate triangulate(convex);

  float totalAngle = 0.f;
  for (auto p : triangulate.getPointAngles()) {
    totalAngle += p;
    EXPECT_LT(p, glm::pi<float>());
  }
  EXPECT_FLOAT_EQ(totalAngle, glm::pi<float>() * 2.f);
}

TEST(Test, TriangulateDonut) {

  std::vector<glm::vec2> donut = {
      {2.0, 2.0}, {2.0, 6.0}, {6.0, 6.0}, {6.0, 2.0}, {4.0, 2.0}, {4.0, 3.0},
      {5.0, 3.0}, {5.0, 5.0}, {3.0, 5.0}, {3.0, 3.0}, {4.0, 3.0}, {4.0, 2.0}};

  auto faceList = Geometry::triangulate(donut);

  Geometry::writeSvg(faceList, donut, testDir() / "TriangulateDonut.svg");
}

auto main(int argc, char **argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}