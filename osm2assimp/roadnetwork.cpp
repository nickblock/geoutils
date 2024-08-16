#include "roadnetwork.h"
#include "svg.h"
#include <glm/ext/scalar_constants.hpp>

namespace GeoUtils {

void Spline::append(const glm::vec2 &p) { mVertices.push_back(p); }
Line Spline::segment(int idx) { return {mVertices[idx], mVertices[idx + 1]}; }
int Spline::numSegments() { return mVertices.size() - 1; }
void Spline::insertJoin(int segmentIdx, const SplineJoin &join) {
  auto &joins = mJoins[segmentIdx];
  joins.push_back(join);

  if (joins.size() > 1) {

    auto s = mVertices[segmentIdx];
    std::sort(joins.begin(), joins.end(), [s](auto &a, auto &b) {
      return glm::distance(s, a.intersection) <
             glm::distance(s, b.intersection);
    });
  }
}

std::optional<SplineJoin>
Spline::getfirstJoin(const std::unordered_set<glm::vec2> &usedPoints) {

  for (auto &joinList : mJoins) {
    for (auto &join : joinList.second) {
      if (usedPoints.find(join.intersection) == usedPoints.end()) {
        return join;
      }
    }
  }
  return {};
}

std::optional<Spline::PointsAndNextJoin>
Spline::getSplineToNextJoin(const SplineJoin &inputJoin) {
  auto segmentIdx = inputJoin.join.segmentIdx;

  std::vector<glm::vec2> points;
  points.push_back(inputJoin.intersection);

  do {
    auto joinIt = mJoins.find(segmentIdx);
    if (joinIt != mJoins.end()) {
      auto &joins = joinIt->second;
      float inputDist =
          glm::distance(mVertices[segmentIdx], inputJoin.intersection);
      for (auto join = joins.begin(); join < joins.end(); ++join) {
        float joinDist =
            glm::distance(mVertices[segmentIdx], (*join).intersection);
        if (joinDist > inputDist) {

          SplineJoin joinReturn = *join;

          return PointsAndNextJoin{points, joinReturn};
        }
      }
    }
    points.push_back(mVertices[segmentIdx++]);
  } while (segmentIdx < mVertices.size() - 1);

  // this will fail to complete loop for polygon :()
  return {};
}

RoadNetwork::RoadNetwork(const BBox &bbox) {

  // outer perimeter goes anti clockwise
  {
    Spline edge;
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge;
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge;
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge;
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    mRoadEdges.push_back(edge);
  }

  mCenter.x = (bbox.mMin.x + bbox.mMax.x) / 2.f;
  mCenter.y = (bbox.mMin.y + bbox.mMax.y) / 2.f;

  mHashSize += 4;
}
void RoadNetwork::addRoad(const Triangulate::Data &road) {

  Spline roadEdge0;
  Spline roadEdge1;
  for (int i = 0; i < road.mVertices.size() / 2; i++) {
    roadEdge0.append(road.mVertices[i]);
    roadEdge1.append(road.mVertices[road.mVertices.size() / 2 + i]);
  }
  mRoadEdges.push_back(roadEdge0);
  mRoadEdges.push_back(roadEdge1);

  mHashSize += road.mVertices.size();
}

Geometry::DataFlat RoadNetwork::getInternalSpaces() {

  auto internalSpaces = startWIthIntersections();

  auto svg = SVGWriter();
  svg.addPolygons(internalSpaces);

  for (auto &sp : mRoadEdges) {
    svg.addLine(sp.vertices(), "white");
  }

  svg.addCircles(mIntersections, 10);

  // svg.addCircles(joins, 4);
  svg.write(testDir() / std::format("RoadNetwork_debug.svg"));

  return internalSpaces;
}

Geometry::DataFlat RoadNetwork::startWIthIntersections() {
  return createSpaceFromJoins();
}

Geometry::DataFlat RoadNetwork::createSpaceFromJoins() {

  findIntersections();

  Geometry::DataFlat internalSpaces;

  auto getNextJoin = [this]() -> std::optional<SplineJoin> {
    for (int i = 0; i < mRoadEdges.size(); i++) {
      auto join = mRoadEdges[i].getfirstJoin(mUsedPoints);
      if (join.has_value()) {
        return join.value();
      }
    }
    return {};
  };

  bool addedSpace = false;
  do {

    addedSpace = false;
    Geometry::DataFlat newSpace;

    auto maybeJoin = getNextJoin();
    while (maybeJoin.has_value()) {
      auto join = maybeJoin.value();

      auto maybeSplineToNext =
          mRoadEdges[join.join.roadIdx].getSplineToNextJoin(join);

      if (!maybeSplineToNext.has_value()) {
        // abort
        maybeJoin = {};
        newSpace.mVertices.clear();
        addedSpace = true; // keep trying
        mUsedPoints.insert(join.intersection);
        continue;
      }

      auto splineToNext = maybeSplineToNext.value();

      for (auto &p : get<std::vector<glm::vec2>>(splineToNext)) {
        mUsedPoints.insert(p);
        newSpace.mVertices.push_back(p);
      }

      auto nextIntersect = get<SplineJoin>(splineToNext);

      if (isSame(nextIntersect.intersection, newSpace.mVertices[0])) {
        internalSpaces = internalSpaces + newSpace;

        newSpace.mVertices.clear();
        maybeJoin = {};
        addedSpace = true;
      } else {
        maybeJoin = nextIntersect;
      }
    }
  } while (addedSpace);

  return internalSpaces;
}
void RoadNetwork::findIntersections() {

  Geometry::DataFlat internalSpace;

  std::vector<glm::vec2> joins;

  for (int i = 0; i < mRoadEdges.size(); i++) {
    for (int k = 0; k < mRoadEdges[i].numSegments(); k++) {

      SegmentIndex s0{i, k};

      Line testLine = mRoadEdges[i].segment(k);
      for (int j = i + 1; j < mRoadEdges.size(); j++) {
        if (j == i) {
          continue;
        }

        for (int l = 0; l < mRoadEdges[j].numSegments(); l++) {

          SegmentIndex s1{j, l};
          Line targetLine = mRoadEdges[j].segment(l);

          if (auto intersection = lineIntersects2d(testLine, targetLine)) {

            mIntersections.push_back(*intersection);

            float reflex = Triangulate::reflexPoint(testLine[0], *intersection,
                                                    targetLine[1]);
            reflex > 0.f ? mRoadEdges[i].insertJoin(k, {s1, *intersection})
                         : mRoadEdges[j].insertJoin(l, {s0, *intersection});

            joins.push_back(*intersection);
          }
        }
      }
    }
  }
}

} // namespace GeoUtils