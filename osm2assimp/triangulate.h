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
  struct EdgeIdx {
    TVertIdx p0, p1;
  };

  struct Data {
    Data(Geometry::DataFlat &sourcePoly) : source(sourcePoly) {}
    Geometry::DataFlat &source;
    std::vector<float> mPointAngles;
    std::vector<EdgeIdx> mEdges;
  };

  std::unique_ptr<Data> mData;

  Triangulate(Geometry::DataFlat &inputPoly);

  std::unique_ptr<Triangulate::Data> getData() { return std::move(mData); }

  static float reflexPoint(const glm::vec2 &a, const glm::vec2 &b,
                           const glm::vec2 &c);

private:
  void execute();

  std::vector<glm::vec2> mVertices;
  std::vector<TVertIdx> mIndices;
  std::vector<float> mInterPointAngles;
  std::vector<EdgeIdx> mInterEdges;

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