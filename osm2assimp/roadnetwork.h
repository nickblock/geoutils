#include "triangulate.h"
#include "utils.h"

#include <map>
#include <optional>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"

namespace GeoUtils {

struct PointCache {

  using PointIdx = size_t;
  std::vector<glm::vec2> mPoints;
  std::vector<bool> mUsed;

  PointIdx append(const glm::vec2 &p) {
    mPoints.push_back(p);
    mUsed.push_back(false);
    return mPoints.size() - 1;
  }
  const std::vector<glm::vec2> &points() { return mPoints; }

  const glm::vec2 &operator[](PointIdx idx) const { return mPoints[idx]; }
  bool used(PointIdx idx) const { return mUsed[idx]; }
  void setUsed(PointIdx idx, bool used = true) { mUsed[idx] = used; }
};

struct SegmentIndex {
  int roadIdx;
  int segmentIdx;
};

struct SplineJoin {
  SegmentIndex join;
  PointCache::PointIdx intersection;
#if defined DEBUG || defined _DEBUG
  glm::vec2 point;
#endif
};

// a sequence of points making up a side of road(s) or outer perimeters of
// buildings.
class Spline {
public:
  Spline(PointCache &cache);
  void append(const glm::vec2 &p);
  Line segment(int idx = 0);
  int numSegments();

  void insertJoin(int segmentIdx, const SplineJoin &join);

  // return first unused join listed
  std::optional<SplineJoin> getfirstJoin();

  // starting from an inputjoin, run along the spline and return all vertices
  // up to the next output join. Return vertices and join.
  using PointsAndNextJoin = std::tuple<std::vector<glm::vec2>, SplineJoin>;

  std::optional<PointsAndNextJoin>
  getSplineToNextJoin(const SplineJoin &inputJoin);

  const std::vector<glm::vec2> &vertices() { return mVertices; }

protected:
  std::map<int, std::vector<SplineJoin>> mJoins;
  std::vector<glm::vec2> mVertices;
  PointCache &mCache;
};

class RoadNetwork {

public:
  RoadNetwork(const BBox &bbox);

  void addRoad(const Triangulate::Data &road);

  // given the current list of roads obtain a list polygons representing the
  //  space encompassed by the roads
  Geometry::DataFlat getInternalSpaces();

  void writeSvg(const std::filesystem::path &path);

protected:
  Geometry::DataFlat createSpaceFromJoins();
  void findIntersections();

  void appendPolygonToData(const std::vector<glm::vec2> &points);

  std::vector<Spline> mRoadEdges;

  glm::vec2 mCenter;
  size_t mHashSize = 0;

  std::vector<Geometry::DataFlat> mSpaces;

  PointCache mIntersections;

  Geometry::DataFlat mData;

  BBox mBBox;
};
} // namespace GeoUtils