#include "triangulate.h"
#include "utils.h"
#include <filesystem>

namespace GeoUtils {

class SVGWriter {
public:
  SVGWriter(const BBox &bbox, bool yup);
  SVGWriter() = default;
  SVGWriter &addPolygons(const Geometry::DataFlat &data,
                         std::string fill = "white",
                         std::string stroke = "red");
  SVGWriter &addLine(const std::vector<glm::vec2> &line,
                     std::string stroke = "blue");

  SVGWriter &addCircles(const std::vector<glm::vec2> &points, int radius,
                        std::string stroke = "red");
  void write(const std::filesystem::path &path);

protected:
  struct Circle {
    std::string stroke;
    int radius;
    std::vector<glm::vec2> points;
  };

  std::vector<Circle> mCircles;

  struct Polygons {
    std::string stroke;
    std::string fill;
    Geometry::DataFlat data;
  };

  std::vector<Polygons> mPolys;

  struct Lines {
    std::string stroke;
    std::vector<glm::vec2> lines;
  };
  std::vector<Lines> mLines;

  std::filesystem::path mPath;

  float mPrecision = 1e2;

  std::stringstream mSS;

  BBox mBBox;
};
} // namespace GeoUtils