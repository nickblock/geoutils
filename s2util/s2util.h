
#include <regex>

#define _USE_MATH_DEFINES
#include <cmath>

#include "eigenconversion.h"
#include "s2/s2cell.h"
#include "s2/s2cell_id.h"
#include "s2/s2latlng.h"
#include <iostream>
#include <string>
#include <tuple>

using std::string;

namespace GeoUtils {

/// <summary>
/// A class exposing some useful functions concerning s2 cells
/// </summary>
class S2Util {

public:
  using LatLng = std::tuple<double, double>;

  static LatLng parseLatLonString(string latLngStr) {
    int commaPos = latLngStr.find_first_of(",");
    double lat = std::stod(latLngStr.substr(0, commaPos));
    latLngStr = latLngStr.substr(commaPos + 1, latLngStr.size());

    commaPos = latLngStr.find_first_of(",");
    double lon = std::stod(latLngStr.substr(0, commaPos));

    return {lat, lon};
  }

  static LatLng getS2Center(uint64_t id) {
    const S2CellId cellId(id);

    if (cellId.is_valid()) {
      const S2LatLng cellCenter = cellId.ToLatLng();
      return {cellCenter.lat().degrees(), cellCenter.lng().degrees()};
    };

    throw std::invalid_argument("getS2Center, id ot valid");
  }

  static std::vector<LatLng> getS2Corners(uint64_t id) {
    const S2CellId cellId(id);

    if (cellId.is_valid()) {
      std::vector<LatLng> corners(4);

      S2Cell s2Cell(cellId);

      for (int i = 0; i < 4; i++) {
        auto s2Point = s2Cell.GetVertex(i);
        auto s2LatLng = S2LatLng(s2Point);
        corners[i] = {s2LatLng.lat().degrees(), s2LatLng.lng().degrees()};
      }
      return corners;
    }

    throw std::invalid_argument("getS2Center, id ot valid");
  }

  static uint64_t getParent(uint64_t id) {
    const S2CellId cellId(id);

    if (cellId.is_valid()) {
      S2CellId parent = cellId.parent();

      return parent.id();
    }
    return 0;
  }

  static uint64_t getS2IdFromString(std::string str) {
    std::smatch res;
    std::regex reg(".*([0-9a-fA-F]{16}).*");
    std::regex_match(str, res, reg);

    if (res.size() <= 1) {
      throw std::invalid_argument(
          "Failed to find 16 hex characters representing an S2 Cell");
    }

    uint64_t id = std::stoul(res[1], nullptr, 16);

    return id;
  }

  static LatLng LLAToCartesian(LatLng loc, LatLng origin) {
    auto resultV3 = LLAtoNED({std::get<0>(origin), std::get<1>(origin), 0.0},
                             {std::get<0>(loc), std::get<1>(loc), 0.0});

    return {resultV3[0], resultV3[1]};
  }
};

} // namespace GeoUtils