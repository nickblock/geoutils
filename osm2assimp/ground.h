#include "glm/vec2.hpp"
#include "osmfeature.h"
#include "utils.h"
#include <filesystem>
#include <map>
#include <vector>

class aiMesh;

namespace GeoUtils {
class Ground {
public:
  Ground(const std::vector<glm::vec2> &);

  void addFootPrint(const std::vector<double> &points);

  aiMesh *getMesh();

  void writeSvg(const std::filesystem::path &path, float scale);

protected:
  using BoxPoly = std::tuple<BBox, std::vector<glm::vec2>>;
  static constexpr int Box = 0;
  static constexpr int Poly = 1;

  static constexpr int kPrecision = 1000;

  std::vector<glm::vec2> mExtents;
  std::vector<double> mGroundPoints;

  int mAdded = 0;

  BBox mBBox;
};
} // namespace GeoUtils