#include "triangulate.h"
#include "utils.h"

#include <unordered_set>

namespace GeoUtils {

// a sequence of points making up a side of road(s) or outer perimeters of
// buildings.
class Spline {
public:
  void append(const glm::vec2 &p);
  Line segment(int idx = 0);
  int numSegments();

  using Split = std::tuple<int, glm::vec2>; // Segment idx + split point
  std::optional<Split> intersectSegment(const Line &segment);

  // reduces this Spline up to split, returns new spline after split
  // if the split occurs at the very beginning or end, we dont split and return
  // nullopt
  std::optional<Spline> split(const Split &splitPoiint);

  const std::vector<glm::vec2> &vertices() { return mVertices; }

protected:
  std::vector<glm::vec2> mVertices;
};

class RoadNetwork {

public:
  RoadNetwork(const BBox &bbox);

  void addRoad(const Triangulate::Data &road);

  // returns roadIdx and point of intersection (split)
  std::optional<std::tuple<int, Spline::Split>>
  findRoadIntersection(const Line &testSegment);

  // given the current list of roads obtain a list polygons representing the
  //  space encompassed by the roads
  Geometry::DataFlat getInternalSpaces();

protected:
  using TRoadEdge = std::vector<glm::vec2>;

  std::vector<Spline> mRoadEdges;
  std::unordered_set<int> mUsedRoads;

  std::vector<Geometry::DataFlat> mSpaces;
};
} // namespace GeoUtils