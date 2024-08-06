#include "triangulate.h"
#include <format>
#include <iostream>

namespace GeoUtils {

Triangulate::Triangulate(const std::span<glm::vec2> &vertices)
    : mVertices(vertices) {

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
  mEdges.clear();
  mPointAngles.clear();
}
void Triangulate::findPolyPerimeter() {

  mEdges.resize(mVertices.size() - mRemovedVertices.size());
  mPointAngles.resize(mVertices.size());

  int currentVertex = firstVertex();

  EdgeIdx lastEdge = {lastVertex(), currentVertex};
  for (int i = 0; i < mEdges.size(); i++) {

    EdgeIdx &curEdge = mEdges[i];

    if (i + 1 != mEdges.size()) {
      curEdge = {.p0 = currentVertex, .p1 = nextVertex(currentVertex)};
    } else {
      curEdge = {.p0 = currentVertex, .p1 = firstVertex()};
    }
    currentVertex = curEdge.p1;

    mPointAngles[currentVertex] = reflexPoint(lastEdge, curEdge);
    lastEdge = curEdge;
  }
}
void Triangulate::execute() {

  findPolyPerimeter();
  while (findAndRemoveTriangle()) {
    if (mVertices.size() - mRemovedVertices.size() >= 4) {
      clearPolyData();
      findPolyPerimeter();
    } else {
      int first = firstVertex();
      int second = nextVertex(first);
      int third = nextVertex(second);
      Tri lastTri = {first, second, third};
      mTris.push_back(lastTri);
      break;
    }
  }
}

int Triangulate::firstVertex() { return nextVertex(-1); }
int Triangulate::lastVertex() {
  int last = mVertices.size() - 1;

  while (mRemovedVertices.find(last) != mRemovedVertices.end()) {
    last--;
    assert(last != 0);
  }
  return last;
}
int Triangulate::nextVertex(int currentVertex) {
  int next = ++currentVertex;
  while (mRemovedVertices.find(next) != mRemovedVertices.end()) {
    next++;
    assert(next < mVertices.size());
  }
  return next;
}

bool Triangulate::findAndRemoveTriangle() {
  if (mEdges.size() < 4) {
    return false;
  }

  for (int i = 0; i < mEdges.size(); i++) {
    auto edge0 = mEdges[i];
    EdgeIdx edge1;
    if (i + 1 < mEdges.size()) {
      edge1 = mEdges[i + 1];
    } else {
      edge1 = mEdges[0];
    }

    if (mPointAngles[edge0.p1] > 0.f) {
      continue;
    }

    Tri tri = {
        edge0.p0,
        edge0.p1,
        edge1.p1,
    };

    EdgeIdx testEdge{edge0.p0, edge1.p1};

    float reflex = reflexPoint(edge0, testEdge);

    float reflexCorner = mPointAngles[edge0.p0];

    auto opp = (reflex > 0.f && reflexCorner > 0.f) ||
               (reflex < 0.0 && reflexCorner < 0.0f);

    if (!opp)
      continue;

    if (!checkEdgeIntersection(testEdge)) {
      clipTriangle(edge0, edge1);
      mTris.push_back(tri);
      return true;
    }
  }
  return false;
}

void Triangulate::clipTriangle(const EdgeIdx &edge0, const EdgeIdx &edge1) {
  mRemovedVertices.insert(edge0.p1);
}
bool Triangulate::checkEdgeIntersection(const EdgeIdx &edge) {
  Line testLine = {mVertices[edge.p0], mVertices[edge.p1]};

  auto lineDir = glm::normalize(testLine[1] - testLine[0]);
  testLine[0] += lineDir * glm::epsilon<float>();
  testLine[1] -= lineDir * glm::epsilon<float>();

  for (auto &side : mEdges) {

    if (side.p0 == edge.p0 || side.p0 == edge.p1 || side.p1 == edge.p0 ||
        side.p1 == edge.p1) {
      continue;
    }

    Line polyEdge = {mVertices[side.p0], mVertices[side.p1]};

    glm::vec2 intersection;

    auto result = lineIntersects2d(testLine, polyEdge, &intersection);

    if (result)
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