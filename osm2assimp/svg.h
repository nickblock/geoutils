#include "triangulate.h"
#include "utils.h"
#include <filesystem>

namespace GeoUtils {

class SVGWriter {
public:
  SVGWriter &addPolygons(const Geometry::DataFlat &data,
                         std::string fill = "white",
                         std::string stroke = "red");
  SVGWriter &addLine(const std::vector<glm::vec2> &line,
                     std::string stroke = "blue");
  void write(const std::filesystem::path &path);

protected:
  BBox mBBox;
  std::filesystem::path mPath;

  float mPrecision = 1e2;

  std::stringstream mSS;
};
} // namespace GeoUtils