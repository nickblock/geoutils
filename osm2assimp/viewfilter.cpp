#include "viewfilter.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "osmfeature.h"
#include "s2/s2cell.h"
#include "s2/s2latlng.h"
#include "s2/s2point.h"

namespace GeoUtils {

using std::make_unique;

TypeFilter::TypeFilter(int filter) : mFilter(filter) {}

bool TypeFilter::include(const osmium::Way &way) const {
  return OSMFeature::determineTypeFromWay(way) & mFilter;
}

BoundFilter::BoundFilter(const osmium::Box &box) : mBox(box) {}

bool BoundFilter::include(const osmium::Way &way) const {
  for (const auto &node : way.nodes()) {
    if (mBox.contains(node.location())) {
      return true;
    }
  }
  return false;
}

S2CellFilter::S2CellFilter(uint64_t id)
    : mS2Cell(make_unique<S2Cell>(S2CellId(id))) {}

bool S2CellFilter::include(const osmium::Way &way) const {
  for (const auto &node : way.nodes()) {
    if (mS2Cell->Contains(S2Point(S2LatLng::FromDegrees(
            node.location().lat(), node.location().lon())))) {
      return true;
    }
  }
  return false;
}

WayIDFilter::WayIDFilter(const std::string &idList) {

  int commaPos = idList.find_first_of(',');
  int begin = 0;
  while (commaPos != std::string::npos) {
    auto subStr = idList.substr(begin, commaPos - begin);
    mIds.insert(std::stol(subStr));

    begin = commaPos + 1;
    commaPos = idList.find_first_of(',', begin);
  }
  auto subStr = idList.substr(begin);
  mIds.insert(std::stol(subStr));
}

bool WayIDFilter::include(const osmium::Way &way) const {
  if (mIds.find(way.id()) != mIds.end()) {
    return true;
  } else {
    return false;
  }
}
} // namespace GeoUtils