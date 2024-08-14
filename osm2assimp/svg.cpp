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
    f << mSS.str();

    f << "</svg>" << std::endl;
  }
}

SVGWriter &SVGWriter::addPolygons(const Geometry::DataFlat &data,
                                  std::string fill, std::string stroke) {

  for (auto &p : data.mVertices) {
    mBBox.add(glm::vec3{p, 0.0} * mPrecision);
  }

  for (auto &face : data.mFaces) {

    mSS << "<polygon points=\"";
    for (auto &idx : face) {
      auto &p = data.mVertices[idx];
      mSS << std::format("{},{} ", (p.x * mPrecision), (p.y * mPrecision))
          << std::endl;
    }
    mSS << "\" fill=\"" << fill << "\" stroke=\"" << stroke << "\" />"
        << std::endl;
  }
  return *this;
}
SVGWriter &SVGWriter::addLine(const std::vector<glm::vec2> &line,
                              std::string stroke) {

  for (auto &p : line) {
    mBBox.add(glm::vec3{p, 0.0} * mPrecision);
  }
  mSS << "<polyline points=\"";
  for (auto &p : line) {
    mSS << std::format("{},{} ", p.x * mPrecision, p.y * mPrecision);
  }

  mSS << "\" fill=\"none\" stroke=\"" << stroke << "\"  />" << std::endl;
  return *this;
}
} // namespace GeoUtils
