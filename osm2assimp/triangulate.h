
#include "geometry.h"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <set>
#include <span>
#include <vector>

namespace GeoUtils {

class Triangulate {

public:
  using Tri = std::array<int, 3>;

  Triangulate(const std::span<glm::vec2> &vertices);

  const std::vector<float> &getPointAngles() { return mPointAngles; }
  const std::vector<Tri> &getTriangles() { return mTris; }

  static float reflexPoint(const glm::vec2 &a, const glm::vec2 &b,
                           const glm::vec2 &c);

private:
  int firstVertex();
  int lastVertex();
  int nextVertex(int currentVertex);
  const std::span<glm::vec2> &mVertices;
  std::set<int> mRemovedVertices;

  std::vector<Tri> mTris;
  std::vector<float> mPointAngles;

  struct EdgeIdx {
    int p0, p1;
  };

  std::vector<EdgeIdx> mEdges;

  float angleBetweenEdges(const EdgeIdx &edge0, const EdgeIdx &edge1);
  float reflexPoint(const EdgeIdx &edge0, const EdgeIdx &edge1);
  bool findAndRemoveTriangle();
  void clipTriangle(const EdgeIdx &edge0, const EdgeIdx &edge1);
  void findPolyPerimeter();
  void clearPolyData();
  void execute();

  // returns true if edge intersects with other edges of polygon
  bool checkEdgeIntersection(const EdgeIdx &edge);
};

} // namespace GeoUtils