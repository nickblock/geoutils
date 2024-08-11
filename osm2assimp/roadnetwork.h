#include "triangulate.h"

namespace GeoUtils {

class RoadNetwork {

public:
  RoadNetwork() = default;

  void addRoad(const Triangulate::Data &road);

  // given the current list of roads obtain a list polygons representing the
  //  space encompassed by the roads
  Geometry::DataFlat getInternalSpaces();
};
} // namespace GeoUtils