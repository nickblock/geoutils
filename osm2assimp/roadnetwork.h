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

// // https://www.shadertoy.com/view/MdcfDj
// constexpr long M1 = 1597334677; // 1719413*929
// constexpr long M2 = 3812015801; // 140473*2467*11

// struct Vec2cHasher {
//   std::size_t operator()(const glm::vec2 &q) const {
//     auto p = q * glm::vec2(M1, M2);
//     size_t n = pow(p.x, p.y);
//     n = n * (pow(n, (n >> 15)));
//     return n * (1.0 / float(0xffffffffU));
//   }
// };
using PointSet = std::unordered_set<glm::vec2>;
// a sequence of points making up a side of road(s) or outer perimeters of
// buildings.
class Spline {
public:
  void append(const glm::vec2 &p);
  Line segment(int idx = 0);
  int numSegments();

  void insertJoin(int segmentIdx, const SplineJoin &join);

  using Split = std::tuple<int, glm::vec2>; // Segment idx + split point
  std::optional<Split> intersectSegment(const Line &segment);

  // reduces this Spline up to split, returns new spline after split
  // if the split occurs at the very beginning or end, we dont split and return
  // nullopt
  std::optional<Spline> split(const Split &splitPoiint);

  const std::vector<glm::vec2> &vertices() { return mVertices; }

  // return first unused join listed
  std::optional<SplineJoin> getfirstJoin(const PointSet &usedPoints);

  // starting from an inputjoin, run along the spline and return all vertices
  // up to the next output join. Return vertices and join.
  std::tuple<std::vector<glm::vec2>, SplineJoin>
  getSplineToNextJoin(const SplineJoin &inputJoin);

protected:
  std::map<int, std::vector<SplineJoin>> mJoins;
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
  size_t hashPoint(const glm::vec2 &);
  Geometry::DataFlat walkTheLines();
  Geometry::DataFlat startWIthIntersections();
  Geometry::DataFlat createSpaceFromJoins();
  void findIntersections();

  using TRoadEdge = std::vector<glm::vec2>;

  std::vector<Spline> mRoadEdges;
  std::unordered_set<int> mUsedRoads;

  glm::vec2 mCenter;
  size_t mHashSize = 0;

  std::vector<Geometry::DataFlat> mSpaces;

  PointSet mUsedPoints;
};
} // namespace GeoUtils