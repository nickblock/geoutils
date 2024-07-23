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

  void addSubtraction(const OSMFeature &feature);

  aiMesh *getMesh();

  void writeSvg(const std::filesystem::path &path, float scale);

protected:
  using BoxPoly = std::tuple<BBox, std::vector<glm::vec2>>;
  static constexpr int Box = 0;
  static constexpr int Poly = 1;

  std::vector<glm::vec2> mExtents;
  std::vector<BoxPoly> mSubtractions; // buildings / roads we want subtracted
                                      // from the ground mesh

  std::vector<OSMFeature> mFeatures;

  int mAdded = 0;

  BBox mBBox;
};
} // namespace GeoUtils