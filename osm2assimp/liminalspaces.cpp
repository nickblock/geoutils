#include "liminalspaces.h"
#include "svg.h"
#include <format>
#include <glm/ext/scalar_constants.hpp>

namespace GeoUtils {

Space::Space(PointCache &cache) : mCache(cache) {};
void Space::append(const glm::vec2 &p) {
  mVertices.push_back(p);
  mBBox.add({p, 0.f});
}
Line Space::segment(int idx) {
  if (idx < mVertices.size() - 1) {
    return {mVertices[idx], mVertices[idx + 1]};
  } else if (idx == mVertices.size() - 1) {
    return {mVertices[idx], mVertices[0]};
  } else {
    idx = idx % mVertices.size();
    int idx1 = idx + 1;
    if (idx == mVertices.size() - 1) {
      idx1 = 0;
    }
    return {mVertices[idx], mVertices[idx1]};
  }
}
int Space::numSegments() { return mVertices.size(); }
bool Space::isLoop() {
  return isSame(mVertices[mVertices.size() - 1],
                mVertices[mVertices.size() / 2]);
}
void Space::insertJoin(int segmentIdx, const SpaceJoin &join) {

  // if the join lands on the segment start, move to previous segment
  if (isSame(mCache[join.intersection], mVertices[segmentIdx])) {
    if (segmentIdx == numSegments() - 1) {
      segmentIdx = 0;
    } else {
      segmentIdx++;
    }
  }
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

std::optional<SpaceJoin> Space::getfirstJoin() {

  for (auto &joinList : mJoins) {
    for (auto &join : joinList.second) {
      if (!mCache.used(join.intersection)) {
        return join;
      }
    }
  }
  return {};
}

std::optional<SpaceJoin>
Space::findJoinAtSegment(int segmentIdx, std::optional<glm::vec2> afterPoint) {
  auto it = mJoins.find(segmentIdx);
  if (it != mJoins.end()) {
    if (afterPoint) {
      auto &joins = it->second;
      float inputDist = glm::distance(mVertices[segmentIdx], *afterPoint);
      for (auto join = joins.begin(); join < joins.end(); ++join) {
        float joinDist =
            glm::distance(mVertices[segmentIdx], mCache[(*join).intersection]);
        if (joinDist > inputDist) {
          return *join;
        }
      }
    } else {
      return it->second[0];
    }
  }
  return {};
}

std::optional<Space::PointsAndNextJoin>
Space::getSpaceToNextJoin(const SpaceJoin &inputJoin) {
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

LiminalSpaces::LiminalSpaces(const BBox &bbox) : mBBox(bbox) {

  // outer perimeter goes anti clockwise
  {
    Space edge(mIntersections);
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMin.y));
    edge.append(glm::vec2(bbox.mMax.x, bbox.mMax.y));
    edge.append(glm::vec2(bbox.mMin.x, bbox.mMax.y));
    mIslands.push_back(edge);
  }

  mCenter.x = (bbox.mMin.x + bbox.mMax.x) / 2.f;
  mCenter.y = (bbox.mMin.y + bbox.mMax.y) / 2.f;

  mHashSize += 4;
}
void LiminalSpaces::addIslands(const Triangulate::Data &island,
                               const std::string &name) {

  Space roadEdge0(mIntersections);
  for (int i = 0; i < island.mVertices.size(); i++) {
    roadEdge0.append(island.mVertices[i]);
  }
  roadEdge0.setName(name);
  mIslands.push_back(roadEdge0);
}

void LiminalSpaces::writeSvg(const std::filesystem::path &path) {

  auto svg = SVGWriter();

  for (auto &sp : mIslands) {
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

void LiminalSpaces::addLoopingSegments(int splineIdx) {

  if (mIslands[splineIdx].isLoop()) {

    auto loopPoint =
        mIslands[splineIdx].vertices()[mIslands[splineIdx].numSegments() / 2];
    auto pointIdx = mIntersections.append(loopPoint);

    auto segmentJoinIdx = mIslands[splineIdx].numSegments() / 2;
    auto endJoinIdx = mIslands[splineIdx].numSegments() - 1;

    SegmentIndex si0{splineIdx, segmentJoinIdx};
    SegmentIndex si1{splineIdx, endJoinIdx};

    mIslands[splineIdx].insertJoin(segmentJoinIdx, {si0, pointIdx});
    mIslands[splineIdx].insertJoin(endJoinIdx, {si1, pointIdx});
  }
}

void LiminalSpaces::appendPolygonToData(const std::vector<glm::vec2> &points) {

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

Geometry::DataFlat LiminalSpaces::getInternalSpaces() {

  return createSpaceFromJoins();
}

Geometry::DataFlat LiminalSpaces::createSpaceFromJoins() {

  findIntersections();

  writeSvg(testDir() / std::format("LiminalSpaces_start.svg"));

  auto getNextJoin = [this]() -> std::optional<SpaceJoin> {
    for (int i = 0; i < mIslands.size(); i++) {
      auto join = mIslands[i].getfirstJoin();
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

    auto resetSearch = [&findNextSpace, &newSpace, &maybeJoin] {
      findNextSpace = true;
      newSpace.clear();
      maybeJoin = {};
    };

    while (maybeJoin) {

      auto join = *maybeJoin;

      newSpace.push_back(mIntersections[join.intersection]);
      mIntersections.setUsed(join.intersection);

      auto &roadJoined = mIslands[join.join.roadIdx];

      auto maybeSpaceToNext = roadJoined.getSpaceToNextJoin(join);

      if (!maybeSpaceToNext) {

        std::cout << "Abort no ongoing join" << std::endl;
        // abort
        resetSearch();
        continue;
      }

      auto [points, nextJoin] = maybeSpaceToNext.value();
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
        resetSearch();
      } else if (mIntersections.used(nextJoin.intersection)) {
        // intersection already used, abort
        resetSearch();
      }
    }
  } while (findNextSpace);

  return mData;
}
void LiminalSpaces::findIntersections() {

  auto segmentIntersection = [this](int roadIdx0, int segmentIdx0, int roadIdx1,
                                    int segmentIdx1) {
    SegmentIndex s0{roadIdx0, segmentIdx0};

    Line testLine = mIslands[roadIdx0].segment(segmentIdx0);

    SegmentIndex s1{roadIdx1, segmentIdx1};
    Line targetLine = mIslands[roadIdx1].segment(segmentIdx1);

    auto intersection = lineIntersects2d(testLine, targetLine);
    if (std::get<bool>(intersection)) {

      float reflex = Triangulate::reflexPoint(
          testLine[0], std::get<glm::vec2>(intersection), targetLine[1]);

      auto pointIdx = mIntersections.append(std::get<glm::vec2>(intersection));

      // Joins define which space and segment they are going to
      // joins are inserted on to the space at the segments they are
      // coming from

      // the direction this takes is consistent due to the source spaces all
      // having a clockwise winding order
      if (reflex > 0) {

        SpaceJoin join = {s1, pointIdx};
#ifdef DEBUG
        join.point = std::get<glm::vec2>(intersection);
#endif

        mIslands[roadIdx0].insertJoin(segmentIdx0, join);
      } else {
        SpaceJoin join = {s0, pointIdx};
#ifdef DEBUG
        join.point = std::get<glm::vec2>(intersection);
#endif

        mIslands[roadIdx1].insertJoin(segmentIdx1, join);
      }
    }
  };

  for (int r0 = 0; r0 < mIslands.size(); r0++) {
    // addLoopingSegments(r0);

    for (int s0 = 0; s0 < mIslands[r0].numSegments(); s0++) {

      for (int s1 = s0; s1 < mIslands[r0].numSegments() - 2; s1++) {
        // search non adjacent segments of same spline
        int otherSeg = s1 + 2;
        if (s0 == 0 && otherSeg == mIslands[r0].numSegments() - 1) {
          continue;
        }
        segmentIntersection(r0, s0, r0, otherSeg);
      }

      for (int r1 = r0 + 1; r1 < mIslands.size(); r1++) {

        if (mIslands[r0].bbox().overlaps(mIslands[r1].bbox())) {

          for (int s1 = 0; s1 < mIslands[r1].numSegments(); s1++) {
            segmentIntersection(r0, s0, r1, s1);
          }
        }
      }
    }
  }
}

} // namespace GeoUtils