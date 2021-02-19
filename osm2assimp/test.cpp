#include "gtest/gtest.h"

#include "geomconvert.h"
#include "ground.h"
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

  std::vector<glm::vec2> clip = {
      {2.5f, 2.5f},
      {2.5f, 17.5f},
      {17.5f, 17.5f},
      {17.5f, 2.5f}};

  ground.addSubtraction(clip);

  AssimpWriter writer;

  writer.addMesh(ground.getMesh());

  EXPECT_EQ(0, writer.write("/tmp/ground.fbx"));
}

auto main(int argc, char **argv) -> int
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}