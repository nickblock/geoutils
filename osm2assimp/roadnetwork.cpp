#include "roadnetwork.h"
#include "svg.h"
#include <format>
#include <glm/ext/scalar_constants.hpp>

namespace GeoUtils {

Spline::Spline(PointCache &cache) : mCache(cache) {};
void Spline::append(const glm::vec2 &p) { mVertices.push_back(p); }
Line Spline::segment(int idx) {
  if (idx < mVertices.size() - 1) {
    return {mVertices[idx], mVertices[idx + 1]};
  } else if (idx == mVertices.size() - 1) {
    return {mVertices[idx], mVertices[0]};
  } else {
    assert(false);
    return {};
  }
}
int Spline::numSegments() { return mVertices.size(); }
void Spline::insertJoin(int segmentIdx, const SplineJoin &join) {
  auto &joins = mJoins[segmentIdx];
  joins.push_back(join);

  if (joins.size() > 1) {

    auto s = mVertices[segmentIdx];
    std::sort(joins.begin(), joins.end(), [s, this](auto &a, auto &b) {
      return glm::distance(s, mCache[a.intersection]) <
             glm::distance(s, mCache[b.intersection]);
    });
  }
}

std::optional<SplineJoin> Spline::getfirstJoin() {

  for (auto &joinList : mJoins) {
    for (auto &join : joinList.second) {
      if (!mCache.used(join.intersection)) {
        return join;
      }
    }
  }
  return {};
}

std::optional<SplineJoin>
Spline::findJoinAtSegment(int segmentIdx, std::optional<glm::vec2> afterPoint) {
  auto it = mJoins.find(segmentIdx);
  if (it != mJoins.end()) {
    if (afterPoint) {
      auto &joins = it->second;
      float inputDist = glm::distance(mVertices[segmentIdx], *afterPoint);
      for (auto join = joins.begin(); join < joins.end(); ++join) {
        float joinDist =
            glm::distance(mVertices[segmentIdx], mCache[(*join).intersection]);
        if (joinDist > inputDist) {
          ;
          return *join;
        }
      }
    } else {
      return it->second[0];
    }
  }
  return {};
}

std::optional<Spline::PointsAndNextJoin>
Spline::getSplineToNextJoin(const SplineJoin &inputJoin) {
  auto startIdx = inputJoin.join.segmentIdx;
  auto segmentIdx = startIdx;

  std::vector<glm::vec2> points;

  do {
    if (startIdx != segmentIdx) {
      points.push_back(mVertices[segmentIdx]);
    }
    auto intersection =
        segmentIdx == startIdx
            ? std::optional<glm::vec2>(mCache[inputJoin.intersection])
            : std::optional<glm::vec2>{};
    auto join = findJoinAtSegment(segmentIdx, intersection);
    if (join) {
      return PointsAndNextJoin(points, *join);
    }
    segmentIdx++;
    if (segmentIdx == mVertices.size()) {
      segmentIdx = 0;
    }
  } while (segmentIdx != startIdx);

  std::cout << "Failed to find outgoing intersection within poly" << std::endl;
  return {};
}

RoadNetwork::RoadNetwork(const BBox &bbox) : mBBox(bbox) {

  // outer perimeter goes anti clockwise
  {
    Spline edge(mIntersections);
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    mRoadEdges.push_back(edge);
  }

  mCenter.x = (bbox.mMin.x + bbox.mMax.x) / 2.f;
  mCenter.y = (bbox.mMin.y + bbox.mMax.y) / 2.f;

  mHashSize += 4;
}
void RoadNetwork::addRoad(const Triangulate::Data &road) {

  Spline roadEdge0(mIntersections);
  for (int i = 0; i < road.mVertices.size(); i++) {
    roadEdge0.append(road.mVertices[i]);
  }
  mRoadEdges.push_back(roadEdge0);
}

void RoadNetwork::writeSvg(const std::filesystem::path &path) {

  auto svg = SVGWriter();

  for (auto &sp : mRoadEdges) {
    svg.addLine(sp.vertices(), "white", true);
  }

  svg.addCircles(mIntersections.points(), 10);
  svg.addPolygons(mData);

  std::vector<glm::vec2> grid;

  float width = mBBox.mMax.x - mBBox.mMin.x;
  float height = mBBox.mMax.y - mBBox.mMin.y;

  auto gridColor = "#999999";

  for (int y = 0; y < ceil(height); y++) {
    svg.addLine({{0.0f, (float)y}, {(float)width, (float)y}}, gridColor);
  }
  for (int x = 0; x < ceil(width); x++) {

    svg.addLine({{(float)x, 0.0}, {(float)x, (float)height}}, gridColor);
  }

  // svg.addCircles(joins, 4);
  svg.write(path);
}

void RoadNetwork::appendPolygonToData(const std::vector<glm::vec2> &points) {

  if (points.size() < 3) {
    return;
  }
  if (Triangulate(points).checkWindingOrder()) {
    return;
  }
  int lastIdx = mData.mVertices.size();

  Face face(points.size());
  for (int i = 0; i < points.size(); i++) {
    face[i] = lastIdx + i;
    mData.mVertices.push_back(points[i]);
  }
  mData.mFaces.emplace_back(std::move(face));
}

Geometry::DataFlat RoadNetwork::getInternalSpaces() {

  return createSpaceFromJoins();
}

Geometry::DataFlat RoadNetwork::createSpaceFromJoins() {

  findIntersections();

  writeSvg(testDir() / std::format("RoadNetwork_start.svg"));

  auto getNextJoin = [this]() -> std::optional<SplineJoin> {
    for (int i = 0; i < mRoadEdges.size(); i++) {
      auto join = mRoadEdges[i].getfirstJoin();
      if (join.has_value()) {
        mIntersections.setUsed(join->intersection);
        return join.value();
      }
    }
    return {};
  };

  auto findLoop = [](const std::vector<glm::vec2> &poly,
                     const glm::vec2 &point) {
    int find = -1;
    for (int i = 0; i < poly.size(); i++) {
      if (isSame(poly[i], point)) {
        find = i;
        break;
      }
    }
    return find;
  };

  bool findNextSpace = false;
  do {

    findNextSpace = false;
    std::vector<glm::vec2> newSpace;

    auto maybeJoin = getNextJoin();

    while (maybeJoin) {

      auto join = *maybeJoin;

      newSpace.push_back(mIntersections[join.intersection]);
      mIntersections.setUsed(join.intersection);

      auto &roadJoined = mRoadEdges[join.join.roadIdx];

      auto maybeSplineToNext = roadJoined.getSplineToNextJoin(join);

      if (!maybeSplineToNext) {

        std::cout << "Abort no ongoing join" << std::endl;
        // abort
        maybeJoin = {};
        newSpace.clear();
        findNextSpace = true; // keep trying
        continue;
      }

      auto [points, nextJoin] = maybeSplineToNext.value();
      maybeJoin = nextJoin;

      for (auto &p : points) {
        newSpace.push_back(p);
      }

      auto loopIdx = findLoop(newSpace, mIntersections[nextJoin.intersection]);
      if (loopIdx != -1) { // found start intersection, polygon complete

        if (loopIdx > 0) {
          newSpace.erase(newSpace.begin(), newSpace.begin() + loopIdx);
        }
        appendPolygonToData(newSpace);
        newSpace.clear();
        maybeJoin = {};
        findNextSpace = true;
      } else if (mIntersections.used(nextJoin.intersection)) {
        // intersection already used, abort
        maybeJoin = {};
        newSpace.clear();
        findNextSpace = true; // keep trying
      }
    }
  } while (findNextSpace);

  return mData;
}
void RoadNetwork::findIntersections() {

  auto segmentIntersection = [this](int roadIdx0, int segmentIdx0, int roadIdx1,
                                    int segmentIdx1) {
    SegmentIndex s0{roadIdx0, segmentIdx0};

    Line testLine = mRoadEdges[roadIdx0].segment(segmentIdx0);

    SegmentIndex s1{roadIdx1, segmentIdx1};
    Line targetLine = mRoadEdges[roadIdx1].segment(segmentIdx1);

    auto intersection = lineIntersects2d(testLine, targetLine);
    if (std::get<bool>(intersection)) {

      float reflex = Triangulate::reflexPoint(
          testLine[0], std::get<glm::vec2>(intersection), targetLine[1]);

      auto pointIdx = mIntersections.append(std::get<glm::vec2>(intersection));

      if (reflex > 0) {

        // Joins define which spline and segment they are going to
        // joins are inserted on to the spline at the segments they are
        // coming from

        // which way round this is is ensured by the source polygons all
        // having a clockwise winding order

        SplineJoin join = {s1, pointIdx};
#ifdef DEBUG
        join.point = std::get<glm::vec2>(intersection);
#endif

        mRoadEdges[roadIdx0].insertJoin(segmentIdx0, join);
      } else {
        SplineJoin join = {s0, pointIdx};
#ifdef DEBUG
        join.point = std::get<glm::vec2>(intersection);
#endif

        mRoadEdges[roadIdx1].insertJoin(segmentIdx1, join);
      }
    }
  };

  for (int r0 = 0; r0 < mRoadEdges.size(); r0++) {

    for (int s0 = 0; s0 < mRoadEdges[r0].numSegments(); s0++) {

      // // search non adjacent segments of same spline
      // for (int s1 = s0 + 2; s1 <= mRoadEdges[r0].numSegments(); s1++) {
      //   int otherSegment = s1;
      //   if (otherSegment >= mRoadEdges[r0].numSegments()) {
      //     otherSegment -= mRoadEdges[r0].numSegments();
      //   }
      //   segmentIntersection(r0, s0, r0, otherSegment);
      // }

      for (int r1 = r0 + 1; r1 < mRoadEdges.size(); r1++) {

        for (int s1 = 0; s1 < mRoadEdges[r1].numSegments(); s1++) {
          segmentIntersection(r0, s0, r1, s1);
        }
      }
    }
  }
}

} // namespace GeoUtils