#include "svg.h"
#include <fstream>

namespace GeoUtils {
void SVGWriter::write(const std::filesystem::path &path) {

  auto f = std::ofstream(path);

  if (f.is_open()) {

    f << std::format("<svg viewBox=\"{} {} {} {}\" xmlns="
                     "\"http://www.w3.org/2000/svg\">",
                     0, 0, (mBBox.mMax.x - mBBox.mMin.x),
                     (mBBox.mMax.y - mBBox.mMin.y))

      << std::endl;

    float height = mBBox.mMax.y - mBBox.mMin.y;
    for (auto &poly : mPolys) {

      for (auto &face : poly.data.mFaces) {

        f << "<polygon points=\"";
        for (auto &idx : face) {
          auto &p = poly.data.mVertices[idx];
          f << std::format("{},{} ", (p.x * mPrecision),
                           (height - (p.y * mPrecision)))
            << std::endl;
        }
        f << "\" fill=\"" << poly.fill << "\" stroke=\"" << poly.stroke
          << "\" />" << std::endl;
      }
    }

    for (auto &circle : mCircles) {

      for (auto &p : circle.points) {

        f << std::format("<circle r=\"{}\" cx=\"{}\" cy=\"{}\" fill=\"{}\" />",
                         circle.radius, p.x * mPrecision,
                         height - (p.y * mPrecision), circle.stroke)
          << std::endl;
      }
    }

    for (auto &line : mLines) {

      f << "<polyline points=\"";
      for (auto &p : line.lines) {
        f << std::format("{},{} ", p.x * mPrecision,
                         height - (p.y * mPrecision));
      }

      f << "\" fill=\"none\" stroke=\"" << line.stroke << "\"  />" << std::endl;
    }

    f << "</svg>" << std::endl;
  }
}

SVGWriter &SVGWriter::addPolygons(const Geometry::DataFlat &data,
                                  std::string fill, std::string stroke) {

  for (auto &p : data.mVertices) {
    mBBox.add(glm::vec3{p, 0.0} * mPrecision);
  }

  mPolys.push_back({stroke, fill, data});

  return *this;
}
SVGWriter &SVGWriter::addLine(const std::vector<glm::vec2> &line,
                              std::string stroke) {

  for (auto &p : line) {
    mBBox.add(glm::vec3{p, 0.0} * mPrecision);
  }

  mLines.push_back({stroke, line});

  return *this;
}

SVGWriter &SVGWriter::addCircles(const std::vector<glm::vec2> &points,
                                 int radius, std::string stroke) {

  for (auto &p : points) {
    mBBox.add(glm::vec3{p, 0.0} * mPrecision);
  }

  mCircles.push_back({stroke, radius, points});

  return *this;
}
} // namespace GeoUtils
