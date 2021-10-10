#include "gtest/gtest.h"

#include "geomconvert.h"
#include "ground.h"
#include "utils.h"
#include "clipper.hpp"
#include "assimpwriter.h"
#include "glm/glm.hpp"

#include <vector>

using namespace GeoUtils;

TEST(Test, MeshFromLine)
{
  std::vector<glm::vec2> points = {
      {0.0, 0.0},
      {0.0, 10.0},
      {10.0, 20.0}};

  auto results = GeomConvert::meshFromLine(points, 2.0, 0);

  EXPECT_TRUE(results != nullptr);
}

TEST(Test, ClipperTest)
{
  std::vector<glm::vec2> corners = {
      {0.0f, 0.0f},
      {0.0f, 10.0f},
      {10.0f, 10.0f},
      {10.0f, 0.0f}};

  Ground ground(corners);

  std::vector<glm::vec2> clip0 = {
      {2.0f, 2.0f},
      {2.0f, 6.0},
      {6.0, 6.0},
      {6.0, 2.0f},
      {2.0f, 2.0f},
  };

  ground.addSubtraction(OSMFeature(clip0, 10.0, OSMFeature::BUILDING, "clip0"));

  std::vector<glm::vec2> clip1;

  for (auto &p : clip0)
  {
    clip1.push_back(p + glm::vec2(5.0, 5.0));
  };

  ground.addSubtraction(OSMFeature(clip1, 10.0, OSMFeature::BUILDING, "clip1"));

  AssimpWriter writer;

  writer.addMesh(ground.getMesh());

  EXPECT_EQ(0, writer.write("/tmp/ground.fbx"));
}

TEST(Test, ClipperLibIntersect)
{

  std::vector<glm::vec2> clip0 = {
      {2.0f, 2.0f},
      {2.0f, 6.0},
      {6.0, 6.0},
      {6.0, 2.0f}};
  std::vector<glm::vec2> clip1;

  for (auto &p : clip0)
  {
    clip1.push_back(p + glm::vec2(2.0, 2.0));
  };

  auto result = intersectPolygons(clip0, clip1);

  EXPECT_EQ(1, result.size());
  EXPECT_EQ(true, polyOrientation(result[0]));

  for (auto &p : clip1)
  {
    p += glm::vec2(5.0, 5.0);
  };

  result = intersectPolygons(clip0, clip1);

  EXPECT_EQ(2, result.size());
  EXPECT_EQ(true, polyOrientation(result[0]));
  EXPECT_EQ(true, polyOrientation(result[0]));
}

auto main(int argc, char **argv) -> int
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}