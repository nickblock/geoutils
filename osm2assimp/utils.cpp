
#define _USE_MATH_DEFINES
#include <cmath>

#include "utils.h"

#include "convertlatlng.h"
#include "s2util.h"

#include <glm/glm.hpp>

#include <osmium/geom/coordinates.hpp>

#include <format>
#include <fstream>
#include <iostream>
#include <sstream>

namespace GeoUtils {
osmium::Box osmiumBoxFromString(string extentsStr) {
  osmium::Box box;

  osmium::Location locMin, locMax;

  int commaPos = extentsStr.find_first_of(",");
  locMin.set_lon(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

  commaPos = extentsStr.find_first_of(",");
  locMin.set_lat(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

  commaPos = extentsStr.find_first_of(",");
  locMax.set_lon(std::stod(extentsStr.substr(0, commaPos)));
  extentsStr = extentsStr.substr(commaPos + 1, extentsStr.size());

  locMax.set_lat(std::stod(extentsStr));

  box.extend(locMin);
  box.extend(locMax);

  return box;
}

vector<string> getInputFiles(string input) {
  vector<std::string> tokens;
  string token;
  std::istringstream tokenStream(input);
  while (std::getline(tokenStream, token, ',')) {
    if (token.size()) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

osmium::Location refPointFromArg(string refPointStr) {
  osmium::Location location;

  int commaPos = refPointStr.find_first_of(",");
  location.set_lat(std::stod(refPointStr));
  refPointStr = refPointStr.substr(commaPos + 1, refPointStr.size());
  location.set_lon(std::stod(refPointStr.substr(0, commaPos)));

  return location;
}

std::filesystem::path testDir() {
  auto testDir = std::filesystem::temp_directory_path() / "geoutils_test";
  if (!std::filesystem::exists(testDir)) {
    std::filesystem::create_directory(testDir);
  }
  return testDir;
}

vector<glm::vec2> cornersFromBox(const osmium::Box &box) {
  vector<glm::vec2> groundCorners(4);

  osmium::geom::Coordinates bottom_left =
      ConvertLatLngToCoords::to_coords(box.bottom_left());
  osmium::geom::Coordinates top_right =
      ConvertLatLngToCoords::to_coords(box.top_right());
  osmium::geom::Coordinates bottom_right(top_right.x, bottom_left.y);
  osmium::geom::Coordinates top_left(bottom_left.x, top_right.y);

  groundCorners[0] = {bottom_left.x, bottom_left.y};
  groundCorners[1] = {top_left.x, top_left.y};
  groundCorners[2] = {top_right.x, top_right.y};
  groundCorners[3] = {bottom_right.x, bottom_right.y};

  return groundCorners;
}

constexpr int FloatToIntMultiplier = 100000;

BBox bBoxFromPoints2D(const std::vector<glm::vec2> &points) {
  BBox bbox;

  for (auto &p : points) {
    bbox.add(glm::vec3(p, 0.0));
  }

  return bbox;
}

glm::vec3 min(const glm::vec3 &p1, const glm::vec3 &p2) {
  return {
      std::min(p1.x, p2.x),
      std::min(p1.y, p2.y),
      std::min(p1.z, p2.z),
  };
}
glm::vec3 max(const glm::vec3 &p1, const glm::vec3 &p2) {

  return {
      std::max(p1.x, p2.x),
      std::max(p1.y, p2.y),
      std::max(p1.z, p2.z),
  };
}
// bounding box functions
BBox::BBox() { init(); }

BBox::BBox(const std::vector<glm::vec3> &v) {
  init();
  add(v);
}

void BBox::init() {
  mMin.x = std::numeric_limits<float>::max();
  mMin.y = std::numeric_limits<float>::max();
  mMin.z = std::numeric_limits<float>::max();
  mMax.x = std::numeric_limits<float>::min();
  mMax.y = std::numeric_limits<float>::min();
  mMax.z = std::numeric_limits<float>::min();
}

void BBox::add(const std::vector<glm::vec3> &v) {
  for (auto &p : v) {
    add(p);
  }
}

void BBox::add(const glm::vec3 &p) {
  mMin = min(mMin, p);
  mMax = max(mMax, p);
}
void BBox::add(const BBox &bb) {
  mMin = min(mMin, bb.mMin);
  mMax = max(mMax, bb.mMax);
}

glm::vec3 BBox::size() { return mMax - mMin; }
glm::vec3 BBox::fraction(const glm::vec3 &in) { return (in - mMin) / size(); }

bool BBox::overlaps(const BBox &other) const {
  for (int i = 0; i < mMin.length(); i++) {
    bool overlap =
        this->mMax[i] >= other.mMin[i] && other.mMax[i] > this->mMin[i];
    if (!overlap) {
      return false;
    }
  }
  return true;
}

auto BBox::transform(const glm::mat4 &mat) -> BBox {
  BBox tBbox;

  tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMin.y, mMin.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMax.y, mMax.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMin.y, mMin.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMax.y, mMin.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMin.y, mMax.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMin.x, mMax.y, mMax.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMin.y, mMax.z, 1.0f)));
  tBbox.add(glm::vec3(mat * glm::vec4(mMax.x, mMax.y, mMin.z, 1.0f)));

  return tBbox;
}

void writeSvg(const std::vector<glm::vec2> &points, int scale,
              const std::filesystem::path &filepath) {

  std::ofstream file = std::ofstream(filepath);

  glm::vec2 min{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  glm::vec2 max{std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min()};

  for (auto &p : points) {
    min.x = std::min(p.x, min.x);
    min.y = std::min(p.y, min.y);
    max.x = std::max(p.x, max.x);
    max.y = std::max(p.y, max.y);
  }

  min.x -= 1;
  min.y -= 1;
  max.x += 1;
  max.y += 1;

  file << std::format("<svg viewBox=\"{} {} {} {}\" xmlns="
                      "\"http://www.w3.org/2000/svg\">",
                      0, 0, (max.x - min.x) * scale, (max.y - min.y) * scale)
       << std::endl;
  file << "<polygon points=\"";
  for (auto &p : points) {
    file << std::format("{},{} ", (p.x - min.x) * scale, (p.y - min.y) * scale)
         << std::endl;
  }

  file << "\" fill=\"none\" stroke=\"white\" />" << std::endl;

  file << "</svg>" << std::endl;
}
} // namespace GeoUtils