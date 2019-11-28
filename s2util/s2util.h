
#include <regex>
#include "s2/s2cell.h"
#include "s2/s2latlng.h"
#include "s2/s2cell_id.h"
#include <tuple>

namespace GeoUtils {

/// <summary>
/// A class exposing some useful functions concerning s2 cells 
/// </summary>
class S2Util {

public:
  
  using LatLng = std::tuple<double, double>;
  
  static LatLng getS2Center(uint64_t id)
  {
    const S2CellId cellId(id);

    if(cellId.is_valid()) {
      const S2LatLng cellCenter = cellId.ToLatLng();
      return {cellCenter.lat().degrees(),cellCenter.lng().degrees()};
    };

    return {0.0,0.0};
  }

  static uint64_t getParent(uint64_t id)
  {
    const S2CellId cellId(id);

    if(cellId.is_valid()) 
    {
      S2CellId parent = cellId.parent();

      return parent.id();
    }
    return 0;
  }

  static uint64_t getS2IdFromString(std::string str)
  {
    std::smatch res; 
    std::regex reg(".*([0-9a-f]{16}).*");
    std::regex_match (str, res, reg);

    if (res.size() <= 1) {
      throw std::invalid_argument("Failed to find 16 hex characters representing an S2 Cell");
    }

    uint64_t id = std::stoul(res[1], nullptr, 16);

    return id;
  }
};

}