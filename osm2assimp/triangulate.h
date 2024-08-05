
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <span>
#include <vector>

namespace GeoUtils {

class Triangulate {

public:
  Triangulate(const std::span<glm::vec2> &vertices);

  const std::vector<float> &getPointAngles() { return mPointAngles; }

private:
  using Tri = std::array<glm::vec2, 3>;

  const std::span<glm::vec2> &mInputVertices;
  std::vector<glm::vec2> mVertices;

  std::vector<Tri> mTris;
  std::vector<float> mPointAngles;

  struct EdgeIdx {
    int p0, p1;
  };

  std::vector<EdgeIdx> mEdges;

  float reflexPoint(const EdgeIdx &edge0, const EdgeIdx &edge1);
  void removeTriangle();
  void getOuterEdges();
  void execute();
};

} // namespace GeoUtils