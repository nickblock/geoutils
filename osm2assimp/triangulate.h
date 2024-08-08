
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
  using VertIdxType = uint32_t;
  using Tri = std::array<VertIdxType, 3>;

  Triangulate(const std::span<glm::vec2> &vertices);

  const std::vector<Tri> &getTriangles() { return mTris; }
  const std::vector<glm::vec2> &getVertices() { return mNewVertices; }

private:
  void execute();

  const std::span<glm::vec2> &mVertices;
  std::vector<glm::vec2> mNewVertices;
  std::vector<Tri> mTris;
};

} // namespace GeoUtils