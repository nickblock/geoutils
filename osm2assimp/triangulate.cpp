#include "triangulate.h"
#include <format>
#include <glm/ext/scalar_constants.hpp>
#include <iostream>
#include <algorithm>

namespace GeoUtils {

Triangulate::Triangulate(const Geometry::DataFlat &inputPoly)
    : mData(std::make_unique<Data>(inputPoly)) {

  mData->mFaces.clear();
  mVertices.insert(mVertices.begin(), inputPoly.mVertices.begin(),
                   inputPoly.mVertices.end());

  mIndices.resize(mVertices.size());
  for (TVertIdx i = 0; i < mVertices.size(); i++) {
    mIndices[i] = i;
  }
  execute();
}

float Triangulate::angleBetweenEdges(const EdgeIdx &edge0,
                                     const EdgeIdx &edge1) {
  glm::vec2 l0 = mVertices[edge0.p1] - mVertices[edge0.p0];
  glm::vec2 l1 = mVertices[edge1.p1] - mVertices[edge1.p0];

  auto dot = glm::dot(l0, l1);
  auto cross = l0.x * l1.y - l0.y * l1.x;

  return cross * dot;
}

float Triangulate::reflexPoint(const glm::vec2 &a, const glm::vec2 &b,
                               const glm::vec2 &c) {
  return (b.x - a.x) * (c.y - b.y) - (c.x - b.x) * (b.y - a.y);
}

float Triangulate::reflexPoint(const EdgeIdx &edge0, const EdgeIdx &edge1) {
  auto &a = mVertices[edge0.p0];
  auto &b = mVertices[edge0.p1];
  auto &c = mVertices[edge1.p1];

  return reflexPoint(a, b, c);
}

void Triangulate::clearPolyData() {
  mInterEdges.clear();
  mInterPointAngles.clear();
}
void Triangulate::findPolyPerimeter() {

  mInterEdges.resize(mVertices.size());
  mInterPointAngles.resize(mVertices.size());

  EdgeIdx lastEdge = {(TVertIdx)mVertices.size() - 1, (TVertIdx)0};

  for (TVertIdx i = 0; i < mVertices.size() - 1; i++) {

    EdgeIdx &curEdge = mInterEdges[i];

    if (i + 1 != mInterEdges.size()) {
      curEdge = {.p0 = i, .p1 = i + 1};
    } else {
      curEdge = {.p0 = i, .p1 = 0};
    }

    mInterPointAngles[i] = reflexPoint(lastEdge, curEdge);
    lastEdge = curEdge;
  }
  mInterEdges[mVertices.size() - 1] = {(TVertIdx)mVertices.size() - 1, 0};
  mInterPointAngles[mVertices.size() - 1] = reflexPoint(lastEdge, {0, 1});
}
void Triangulate::execute() {

  findPolyPerimeter();

  if (!checkWindingOrder()) {
    clearPolyData();
    std::reverse(mVertices.begin(), mVertices.end());
    std::reverse(mData->mVertices.begin(), mData->mVertices.end());
    findPolyPerimeter();
  }

  // after the first pass we have thee inital outer perimater, edges, angles at
  // each vertex
  mData->mEdges.insert(mData->mEdges.begin(), mInterEdges.begin(),
                       mInterEdges.end());
  mData->mPointAngles.insert(mData->mPointAngles.begin(),
                             mInterPointAngles.begin(),
                             mInterPointAngles.end());

  // susequently we clip triangles
  while (findAndRemoveTriangle()) {
    if (mVertices.size() >= 4) {
      clearPolyData();
      findPolyPerimeter();
    } else {
      mData->mFaces.push_back({mIndices[0], mIndices[1], mIndices[2]});
      break;
    }
  }
}

bool Triangulate::checkWindingOrder() {
  int negCount = 0;
  for (int i = 0; i < mInterPointAngles.size(); i++) {
    if (mInterPointAngles[i] != 0.0f) {
      mInterPointAngles[i] > 0 ? negCount-- : negCount++;
    }
  }
  return negCount > 0;
}
bool Triangulate::findAndRemoveTriangle() {
  if (mInterEdges.size() < 4) {
    return false;
  }

  for (int i = 0; i < mInterEdges.size(); i++) {
    auto edge0 = mInterEdges[i];
    EdgeIdx edge1;
    if (i + 1 < mInterEdges.size()) {
      edge1 = mInterEdges[i + 1];
    } else {
      edge1 = mInterEdges[0];
    }

    if (mInterPointAngles[edge0.p1] > 0.f) {
      continue;
    }

    Face tri = {
        edge0.p0,
        edge0.p1,
        edge1.p1,
    };

    EdgeIdx testEdge{edge0.p0, edge1.p1};

    float reflex = reflexPoint(edge0, testEdge);

    float reflexCorner = mInterPointAngles[edge0.p0];

    auto opp = (reflex > 0.f && reflexCorner > 0.f) ||
               (reflex < 0.0 && reflexCorner < 0.0f);

    if (!opp)
      continue;

    if (!checkEdgeIntersection(testEdge)) {
      clipTriangle(edge0, edge1);
      return true;
    }
  }
  return false;
}

void Triangulate::clipTriangle(const EdgeIdx &edge0, const EdgeIdx &edge1) {
  mData->mFaces.push_back(
      {mIndices[edge0.p0], mIndices[edge0.p1], mIndices[edge1.p1]});
  mVertices.erase(mVertices.begin() + edge0.p1);
  mIndices.erase(mIndices.begin() + edge0.p1);
}
bool Triangulate::checkEdgeIntersection(const EdgeIdx &edge) {
  Line testLine = {mVertices[edge.p0], mVertices[edge.p1]};

  auto lineDir = glm::normalize(testLine[1] - testLine[0]);
  testLine[0] += lineDir * glm::epsilon<float>();
  testLine[1] -= lineDir * glm::epsilon<float>();

  for (auto &side : mInterEdges) {

    if (side.p0 == edge.p0 || side.p0 == edge.p1 || side.p1 == edge.p0 ||
        side.p1 == edge.p1) {
      continue;
    }

    Line polyEdge = {mVertices[side.p0], mVertices[side.p1]};

    auto intersection = lineIntersects2d(testLine, polyEdge);

    if (intersection)
      return true;

    if (side.p0 != edge.p0 && side.p0 != edge.p1) {
      if (pointOnLine(testLine, polyEdge[0])) {
        return true;
      }
    }

    if (side.p1 != edge.p0 && side.p1 != edge.p1) {
      if (pointOnLine(testLine, polyEdge[1])) {
        return true;
      }
    }
  }
  return false;
}
} // namespace GeoUtils
