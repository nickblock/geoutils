#include "roadnetwork.h"
#include "svg.h"
#include <glm/ext/scalar_constants.hpp>

namespace GeoUtils {

void Spline::append(const glm::vec2 &p) { mVertices.push_back(p); }
Line Spline::segment(int idx) { return {mVertices[idx], mVertices[idx + 1]}; }
int Spline::numSegments() { return mVertices.size() - 1; }
std::optional<Spline::Split> Spline::intersectSegment(const Line &test) {
  for (int i = 0; i < numSegments(); i++) {
    glm::vec2 intersection;
    if (lineIntersects2d(test, segment(i), &intersection)) {
      return Split{i, intersection};
    }
  }
  return {};
}

std::optional<Spline> Spline::split(const Spline::Split &splitPoint) {
  Spline spline;

  auto &point = get<glm::vec2>(splitPoint);
  auto index = get<int>(splitPoint);

  if ((index == 0 &&
       glm::distance(point, mVertices[index]) < glm::epsilon<float>()) ||
      (index == mVertices.size() - 1 &&
       glm::distance(point, mVertices[mVertices.size() - 1]) <
           glm::epsilon<float>()) ||
      (index == 0 && mVertices.size() == 2 &&
       glm::distance(point, mVertices[1]) < glm::epsilon<float>())) {
    return {};
  }

  spline.append(point);
  for (int i = index + 1; i < mVertices.size(); i++) {
    spline.append(mVertices[i]);
  }

  mVertices.resize(index + 2);
  mVertices[index + 1] = point;

  return spline;
}

RoadNetwork::RoadNetwork(const BBox &bbox) {
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

std::optional<std::tuple<int, Spline::Split>>
RoadNetwork::findRoadIntersection(const Line &testLine) {

  Spline::Split split;
  int roadIdx = -1;
  float closestDist = std::numeric_limits<float>::max();
  for (int i = 0; i < mRoadEdges.size(); i++) {

    Spline &otherEdge = mRoadEdges[i];

    auto maybeSplit = otherEdge.intersectSegment(testLine);

    if (maybeSplit.has_value()) {

      // auto reflex = Triangulate::reflexPoint(
      //     testLine[0], get<glm::vec2>(maybeSplit.value()),
      //     otherEdge.segment(get<int>(maybeSplit.value()))[1]);

      // if (reflex <= 0) {
      //   continue;
      // }
      float distance =
          glm::distance(testLine[0], std::get<glm::vec2>(maybeSplit.value()));
      if (distance > glm::epsilon<float>() && distance < closestDist) {
        closestDist = distance;
        roadIdx = i;
        split = maybeSplit.value();
      }
    }
  }
  if (roadIdx != -1) {
    return std::tuple<int, Spline::Split>{roadIdx, split};
  } else {
    return {};
  }
}
Geometry::DataFlat RoadNetwork::getInternalSpaces() {

  Geometry::DataFlat internalSpace;

  auto getNextRoadIdx = [this]() {
    int roadIdx = 0;
    for (roadIdx = 0; roadIdx < mRoadEdges.size(); roadIdx++) {
      if (mUsedRoads.find(roadIdx) == mUsedRoads.end()) {
        break;
      }
    }
    return roadIdx;
  };

  int roadIdx = 0;
  while (roadIdx < mRoadEdges.size()) {

    auto *currentRoad = &mRoadEdges[roadIdx];

    Geometry::DataFlat newSpace;
    int segmentIdx = 0;
    while (segmentIdx < currentRoad->numSegments()) {

      mUsedRoads.insert(roadIdx);
      Line segment = currentRoad->segment(segmentIdx);

      if (newSpace.mVertices.size() &&
          glm::distance(segment[0], newSpace.mVertices[0]) <
              glm::epsilon<float>()) {
        // found start, finished space
        break;
      }

      newSpace.mVertices.push_back(segment[0]);

      auto maybeIntersection = findRoadIntersection(segment);

      if (maybeIntersection.has_value()) {

        auto [intersectionIdx, split] = maybeIntersection.value();

        auto &intersectingEdge = mRoadEdges[intersectionIdx];

        auto intersectingSegment = intersectingEdge.segment(get<int>(split));

        auto reflex = Triangulate::reflexPoint(
            segment[0], get<glm::vec2>(split), intersectingSegment[1]);

        // if (reflex <= 0.f) {

        //   // intersection at wrong winding roder, we ignore these

        //   auto currentEdgeRemaining =
        //       currentRoad->split({segmentIdx, get<glm::vec2>(split)});
        //   if (currentEdgeRemaining.has_value()) {
        //     mRoadEdges.push_back(currentEdgeRemaining.value());
        //   }

        //   // reset the newSpace and start with nexts idx
        //   newSpace.mVertices.clear();
        //   break;
        // }

        auto intersectingEdgeRemaining = intersectingEdge.split(split);

        auto currentEdgeRemaining =
            currentRoad->split({segmentIdx, get<glm::vec2>(split)});
        if (currentEdgeRemaining.has_value()) {
          mRoadEdges.push_back(currentEdgeRemaining.value());

          auto seg = currentEdgeRemaining.value().segment(0);
          std::cout << std::format("Rem Cur : {},{} -> {}, {}", seg[0].x,
                                   seg[0].y, seg[1].x, seg[1].y)
                    << std::endl;
          ;
        }

        if (intersectingEdgeRemaining.has_value()) {
          mRoadEdges.push_back(intersectingEdgeRemaining.value());
          auto seg = intersectingEdgeRemaining.value().segment(0);
          std::cout << std::format("Rem Int : {},{} -> {}, {}", seg[0].x,
                                   seg[0].y, seg[1].x, seg[1].y)
                    << std::endl;
          ;
          roadIdx = mRoadEdges.size() - 1;
          currentRoad = &mRoadEdges[roadIdx];
        } else {
          roadIdx = intersectionIdx;
          currentRoad = &mRoadEdges[intersectionIdx];
        }

        segmentIdx = 0;
      } else {
        segmentIdx++;
      }
    }

    if (newSpace.mVertices.size() > 2) {

      internalSpace = internalSpace + newSpace;

      static int idx = 0;
      auto svg = SVGWriter();
      svg.addPolygons(internalSpace);

      for(auto sp : mRoadEdges) {
        svg.addLine(sp.vertices());
      }
      svg.write(testDir() / std::format("RoadNetwork_{}.svg", idx++));
    }

    roadIdx = getNextRoadIdx();
    segmentIdx = 0;
  }

  return internalSpace;
}
} // namespace GeoUtils