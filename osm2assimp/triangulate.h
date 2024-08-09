#pragma once
#include "geometry.h"
#include <array>
#include <glm/glm.hpp>
#include <span>
#include <vector>

namespace GeoUtils {

class Triangulate {

public:
  Triangulate(const std::span<glm::vec2> &vertices);

  Geometry::DataFlat getData() { return mData; }

private:
  void execute();

  const std::span<glm::vec2> &mVertices;

  Geometry::DataFlat mData;
};

} // namespace GeoUtils