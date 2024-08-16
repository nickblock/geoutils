#include "triangulate.h"
#include "utils.h"

#include <map>
#include <unordered_set>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

namespace GeoUtils {

struct SegmentIndex {
  int roadIdx;
  int segmentIdx;
};

struct SplineJoin {
  SegmentIndex join;
  glm::vec2 intersection;
};

// a sequence of points making up a side of road(s) or outer perimeters of
// buildings.
class Spline {
public:
  void append(const glm::vec2 &p);
  Line segment(int idx = 0);
  int numSegments();

  void insertJoin(int segmentIdx, const SplineJoin &join);

  // return first unused join listed
  std::optional<SplineJoin>
  getfirstJoin(const std::unordered_set<glm::vec2> &usedPoints);

  // starting from an inputjoin, run along the spline and return all vertices
  // up to the next output join. Return vertices and join.
  using PointsAndNextJoin = std::tuple<std::vector<glm::vec2>, SplineJoin>;

  std::optional<PointsAndNextJoin>
  getSplineToNextJoin(const SplineJoin &inputJoin);

  const std::vector<glm::vec2> &vertices() { return mVertices; }

protected:
  std::map<int, std::vector<SplineJoin>> mJoins;
  std::vector<glm::vec2> mVertices;
};

class RoadNetwork {

public:
  RoadNetwork(const BBox &bbox);

  void addRoad(const Triangulate::Data &road);

  // given the current list of roads obtain a list polygons representing the
  //  space encompassed by the roads
  Geometry::DataFlat getInternalSpaces();

protected:
  Geometry::DataFlat startWIthIntersections();
  Geometry::DataFlat createSpaceFromJoins();
  void findIntersections();

  using TRoadEdge = std::vector<glm::vec2>;

  std::vector<Spline> mRoadEdges;
  std::unordered_set<int> mUsedRoads;

  glm::vec2 mCenter;
  size_t mHashSize = 0;

  std::vector<Geometry::DataFlat> mSpaces;

  std::unordered_set<glm::vec2> mUsedPoints;

  std::vector<glm::vec2> mIntersections;

  BBox mBBox;
};
} // namespace GeoUtils