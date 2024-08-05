#include "triangulate.h"

namespace GeoUtils {

Triangulate::Triangulate(const std::span<glm::vec2> &vertices)
    : mInputVertices(vertices) {

  mVertices.insert(mVertices.begin(), mInputVertices.begin(),
                   mInputVertices.end());
  execute();
}

float Triangulate::reflexPoint(const EdgeIdx &edge0, const EdgeIdx &edge1) {
  auto &a = mVertices[edge0.p0];
  auto &b = mVertices[edge0.p1];
  auto &c = mVertices[edge1.p1];

  return (b.x - a.x) * (c.y - b.y) - (c.x - b.x) * (b.y - a.y);
}
void Triangulate::getOuterEdges() {

  mEdges.resize(mVertices.size());
  mPointAngles.resize(mVertices.size());

  EdgeIdx *lastEdge = &mEdges[0];
  for (int i = 1; i < mVertices.size(); i++) {

    EdgeIdx &curEdge = mEdges[i];

    if (i + 1 != mVertices.size()) {
      curEdge = {.p0 = i, .p1 = i + 1};
    } else {
      curEdge = {.p0 = i, .p1 = 0};
    }

    mPointAngles[i] = reflexPoint(*lastEdge, curEdge);
    lastEdge = &curEdge;
  }
  mPointAngles[0] = reflexPoint(*lastEdge, mEdges[0]);
}
void Triangulate::execute() {
  getOuterEdges();
  // for (int i = 0; i < mVertices.size() - 2; i++) {

  //   TriIdx tri{i, i + 1, i + 2};
  // }
}
} // namespace GeoUtils