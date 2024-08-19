#include "roadnetwork.h"
#include "svg.h"
#include <format>
#include <glm/ext/scalar_constants.hpp>

namespace GeoUtils {

Spline::Spline(PointCache &cache) : mCache(cache) {};
void Spline::append(const glm::vec2 &p) { mVertices.push_back(p); }
Line Spline::segment(int idx) { return {mVertices[idx], mVertices[idx + 1]}; }
int Spline::numSegments() { return mVertices.size() - 1; }
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

std::optional<Spline::PointsAndNextJoin>
Spline::getSplineToNextJoin(const SplineJoin &inputJoin) {
  auto segmentIdx = inputJoin.join.segmentIdx;

  std::vector<glm::vec2> points;
  points.push_back(mCache[inputJoin.intersection]);

  do {
    auto joinIt = mJoins.find(segmentIdx);
    if (joinIt != mJoins.end()) {
      auto &joins = joinIt->second;
      float inputDist = glm::distance(mVertices[inputJoin.join.segmentIdx],
                                      mCache[inputJoin.intersection]);
      for (auto join = joins.begin(); join < joins.end(); ++join) {
        float joinDist = glm::distance(mVertices[inputJoin.join.segmentIdx],
                                       mCache[(*join).intersection]);
        if (joinDist > inputDist) {

          mCache.setUsed(join->intersection);

          return PointsAndNextJoin{points, *join};
        }
      }
    }
    points.push_back(mVertices[segmentIdx]);
    segmentIdx++;
  } while (segmentIdx < mVertices.size() - 1);

  // this will fail to complete loop for polygon :()
  return {};
}

RoadNetwork::RoadNetwork(const BBox &bbox) : mBBox(bbox) {

  // outer perimeter goes anti clockwise
  {
    Spline edge(mIntersections);
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge(mIntersections);
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge(mIntersections);
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    mRoadEdges.push_back(edge);
  }
  {
    Spline edge(mIntersections);
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    mRoadEdges.push_back(edge);
  }

  mCenter.x = (bbox.mMin.x + bbox.mMax.x) / 2.f;
  mCenter.y = (bbox.mMin.y + bbox.mMax.y) / 2.f;

  mHashSize += 4;
}
void RoadNetwork::addRoad(const Triangulate::Data &road) {

  Spline roadEdge0(mIntersections);
  Spline roadEdge1(mIntersections);
  for (int i = 0; i < road.mVertices.size() / 2; i++) {
    roadEdge0.append(road.mVertices[i]);
    roadEdge1.append(road.mVertices[road.mVertices.size() / 2 + i]);
  }
  mRoadEdges.push_back(roadEdge0);
  mRoadEdges.push_back(roadEdge1);
}

void RoadNetwork::writeSvg(const std::filesystem::path &path) {

  auto svg = SVGWriter();

  for (auto &sp : mRoadEdges) {
    svg.addLine(sp.vertices(), "white");
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

  bool keepGoing = false;
  do {

    keepGoing = false;
    std::vector<glm::vec2> newSpace;

    auto maybeJoin = getNextJoin();
    if (maybeJoin) {
      std::cout << "New Start " << maybeJoin->point.x << " "
                << maybeJoin->point.y << std::endl;
    }
    while (maybeJoin) {

      auto join = *maybeJoin;
      auto &roadJoined = mRoadEdges[join.join.roadIdx];

      auto maybeSplineToNext = roadJoined.getSplineToNextJoin(join);

      if (!maybeSplineToNext) {

        std::cout << "Abort no ongoing join" << std::endl;
        // abort
        maybeJoin = {};
        newSpace.clear();
        keepGoing = true; // keep trying
        mIntersections.setUsed(join.intersection);
        continue;
      }

      auto splineToNext = *maybeSplineToNext;

      for (auto &p : get<std::vector<glm::vec2>>(splineToNext)) {
        newSpace.push_back(p);
      }

      auto nextIntersect = get<SplineJoin>(splineToNext);

      auto loopIdx =
          findLoop(newSpace, mIntersections[nextIntersect.intersection]);
      if (loopIdx != -1) {

        if (loopIdx > 0) {
          newSpace.erase(newSpace.begin(), newSpace.begin() + loopIdx);
        }
        std::cout << "Added Polygon" << std::endl;
        appendPolygonToData(newSpace);

        newSpace.clear();
        maybeJoin = {};
        keepGoing = true;
      } else {

        maybeJoin = nextIntersect;
      }
    }
  } while (keepGoing);

  writeSvg(testDir() / std::format("RoadNetwork_debug.svg"));

  return mData;
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

            auto pointIdx = mIntersections.append(*intersection);

            if (reflex > 0) {
              SplineJoin join = {s1, pointIdx};
#ifdef DEBUG
              join.point = *intersection;
#endif

              mRoadEdges[i].insertJoin(k, join);
            } else {
              SplineJoin join = {s0, pointIdx};
#ifdef DEBUG
              join.point = *intersection;
#endif

              mRoadEdges[j].insertJoin(l, join);
            }
          }
        }
      }
    }
  }
}

} // namespace GeoUtils