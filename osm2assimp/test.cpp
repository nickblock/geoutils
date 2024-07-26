#include "gtest/gtest.h"

#include "assimpwriter.h"
#include "clipper.hpp"
#include "geometry.h"
#include "glm/glm.hpp"
#include "ground.h"
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
  ground.writeSvg(fs::temp_directory_path() / "ClipperTest.svg", 100.0);

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(fs::temp_directory_path() / "ClipperTest.fbx"));
}

TEST(Test, GroundDonut) {

  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f}, {0.0f, 10.0f}, {10.0f, 10.0f}, {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<double> donut = {2.0, 2.0, 2.0, 6.0, 6.0, 6.0, 6.0, 2.0,
                               4.0, 2.0, 4.0, 3.0, 5.0, 3.0, 5.0, 5.0,
                               3.0, 5.0, 3.0, 3.0, 3.5, 3.0, 3.5, 2.0};

  ground.addFootPrint(donut, 0);

  ground.writeSvg(fs::temp_directory_path() / "GroundDonut.svg", 100.0);

  auto groundMesh = ground.getMesh();
  EXPECT_NE(groundMesh, nullptr);

  AssimpWriter writer;
  writer.addMesh(groundMesh);
  EXPECT_EQ(0, writer.write(fs::temp_directory_path() / "GroundDonut.fbx"));
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
             std::format("{}/ClipperSubtractPoly{}.svg",
                         fs::temp_directory_path().string(), idx++));
  }
}

auto main(int argc, char **argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}