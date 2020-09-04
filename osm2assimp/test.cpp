#include "gtest/gtest.h"

#include "geomconvert.h"
#include "glm/glm.hpp"
#include <vector>

TEST(Test, Spline)
{
  std::vector<glm::vec2> points = {
      {0.0, 0.0},
      {0.0, 10.0},
      {5.0, 20.0}};

  auto results = GeoUtils::GeomConvert::polygonFromSpline(points, 0.5);

  EXPECT_TRUE(results != nullptr);
}

auto main(int argc, char **argv) -> int
{
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}