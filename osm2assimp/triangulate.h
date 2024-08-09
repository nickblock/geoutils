#pragma once
#include "geometry.h"
#include <array>
#include <glm/glm.hpp>
#include <set>
#include <span>
#include <vector>

namespace GeoUtils {

class Triangulate {

public:
  Triangulate(const std::span<glm::vec2> &vertices);

  Geometry::DataFlat getData() { return mData; }

  static float reflexPoint(const glm::vec2 &a, const glm::vec2 &b,
                           const glm::vec2 &c);

private:
  void execute();

  const std::span<glm::vec2> &mVertices;

  Geometry::DataFlat mData;

  TVertIdx firstVertex();
  TVertIdx lastVertex();
  TVertIdx nextVertex(int currentVertex);
  std::set<TVertIdx> mRemovedVertices;

  std::vector<float> mPointAngles;

  struct EdgeIdx {
    TVertIdx p0, p1;
  };

  std::vector<EdgeIdx> mEdges;

  float angleBetweenEdges(const EdgeIdx &edge0, const EdgeIdx &edge1);
  float reflexPoint(const EdgeIdx &edge0, const EdgeIdx &edge1);
  bool findAndRemoveTriangle();
  void clipTriangle(const EdgeIdx &edge0, const EdgeIdx &edge1);
  void findPolyPerimeter();
  void clearPolyData();

  // returns true if edge intersects with other edges of polygon
  bool checkEdgeIntersection(const EdgeIdx &edge);
};

} // namespace GeoUtils