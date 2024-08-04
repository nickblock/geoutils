
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
  using TriIdx = std::array<int, 3>;

  const std::span<glm::vec2> &mVertices;

  std::vector<TriIdx> mTris;
  std::vector<float> mPointAngles;

  struct EdgeIdx {
    int p0, p1;
    float angle;
  };

  std::vector<EdgeIdx> mEdges;

  float edgeAngle(int p0, int p1);
  void getOuterEdges();
  void execute();
};

} // namespace GeoUtils