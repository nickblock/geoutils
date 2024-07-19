#include <vector>
#include <map>
#include "glm/vec2.hpp"
#include "utils.h"
#include "osmfeature.h"

class aiMesh;

namespace GeoUtils
{
  class Ground
  {
  public:
    Ground(const std::vector<glm::vec2> &);

    void addSubtraction(const OSMFeature &feature);

    aiMesh *getMesh();

    void writeSvg(const std::string &path, float scale);

  protected:
    using BoxPoly = std::tuple<BBox, std::vector<glm::vec2>>;
    static constexpr int Box = 0;
    static constexpr int Poly = 1;

    std::vector<glm::vec2> mExtents;
    std::vector<BoxPoly> mSubs;

    std::vector<OSMFeature> mFeatures;

    int mAdded = 0;

    BBox mBBox;
  };
}