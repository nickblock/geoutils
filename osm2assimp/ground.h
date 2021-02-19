#include <vector>
#include "glm/vec2.hpp"
#include "clipper.hpp"
#include "utils.h"

class aiMesh;

using ClipperLib::Path;
using ClipperLib::Paths;
namespace GeoUtils
{
  class Ground
  {
  public:
    Ground(const std::vector<glm::vec2> &);

    void addSubtraction(const std::vector<glm::vec2> &polygon);

    aiMesh *getMesh();

    void writePoly(const std::string &path);

  protected:
    Path mExtents;
    Paths mSubs;

    BBox mBBox;
  };
}