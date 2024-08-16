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
      float inputDist = glm::distance(mVertices[inputJoin.join.segmentIdx],
                                      inputJoin.intersection);
      for (auto join = joins.begin(); join < joins.end(); ++join) {
        float joinDist = glm::distance(mVertices[inputJoin.join.segmentIdx],
                                       (*join).intersection);
        if (joinDist > inputDist) {

          return PointsAndNextJoin{points, *join};
        }
      }
    }
    points.push_back(mVertices[++segmentIdx]);
  } while (segmentIdx < mVertices.size() - 1);

  // this will fail to complete loop for polygon :()
  return {};
}

RoadNetwork::RoadNetwork(const BBox &bbox) : mBBox(bbox) {

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

  auto svg = SVGWriter();

  for (auto &sp : mRoadEdges) {
    svg.addLine(sp.vertices(), "white");
  }

  svg.addCircles(mIntersections, 10);

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
  svg.write(testDir() / std::format("RoadNetwork_start.svg"));

  Geometry::DataFlat internalSpaces;

  auto getNextJoin = [this]() -> std::optional<SplineJoin> {
    for (int i = 0; i < mRoadEdges.size(); i++) {
      auto join = mRoadEdges[i].getfirstJoin(mUsedPoints);
      if (join.has_value()) {
        mUsedPoints.insert(join->intersection);
        return join.value();
      }
    }
    return {};
  };

  bool keepGoing = false;
  do {

    keepGoing = false;
    Geometry::DataFlat newSpace;

    auto maybeJoin = getNextJoin();
    if (maybeJoin) {
      std::cout << "New Start " << maybeJoin->intersection.x << " "
                << maybeJoin->intersection.y << std::endl;
    }
    while (maybeJoin) {
      auto join = *maybeJoin;

      auto &roadJoined = mRoadEdges[join.join.roadIdx];

      auto maybeSplineToNext = roadJoined.getSplineToNextJoin(join);

      if (!maybeSplineToNext) {

        std::cout << "Abort no ongoing join" << std::endl;
        // abort
        maybeJoin = {};
        newSpace.mVertices.clear();
        keepGoing = true; // keep trying
        mUsedPoints.insert(join.intersection);
        continue;
      }

      auto splineToNext = *maybeSplineToNext;

      for (auto &p : get<std::vector<glm::vec2>>(splineToNext)) {
        mUsedPoints.insert(p);
        newSpace.mVertices.push_back(p);
      }

      auto nextIntersect = get<SplineJoin>(splineToNext);

      if (isSame(nextIntersect.intersection, newSpace.mVertices[0])) {

        std::cout << "Added Polygon" << std::endl;
        internalSpaces = internalSpaces + newSpace;

        newSpace.mVertices.clear();
        maybeJoin = {};
        keepGoing = true;
      } else {

        // detect bad loop
        bool bad = false;
        for (auto &p : newSpace.mVertices) {
          if (isSame(p, nextIntersect.intersection)) {
            bad = true;
            break;
          }
        }
        if (!bad) {
          maybeJoin = nextIntersect;
        } else {

          std::cout << "Abort bad loop" << std::endl;
          // abort
          for (int i = 1; i < newSpace.mVertices.size(); i++) {
            mUsedPoints.erase(newSpace.mVertices[i]);
          }
          mUsedPoints.erase(nextIntersect.intersection);
          newSpace.mVertices.clear();
          maybeJoin = {};
          keepGoing = true;
        }
      }
    }
  } while (keepGoing);

  return internalSpaces;
}
void RoadNetwork::findIntersections() {

  Geometry::DataFlat internalSpace;

  for (int i = 0; i < mRoadEdges.size(); i++) {

    for (int k = 0; k < mRoadEdges[i].numSegments(); k++) {

      SegmentIndex s0{i, k};

      Line testLine = mRoadEdges[i].segment(k);
      for (int j = i + 1; j < mRoadEdges.size(); j++) {

        for (int l = 0; l < mRoadEdges[j].numSegments(); l++) {

          SegmentIndex s1{j, l};
          Line targetLine = mRoadEdges[j].segment(l);

          if (auto intersection = lineIntersects2d(testLine, targetLine)) {

            float reflex = Triangulate::reflexPoint(testLine[0], *intersection,
                                                    targetLine[1]);

            mIntersections.push_back(*intersection);

            reflex > 0.f ? mRoadEdges[i].insertJoin(k, {s1, *intersection})
                         : mRoadEdges[j].insertJoin(l, {s0, *intersection});
          }
        }
      }
    }
  }
}

} // namespace GeoUtils