#include "triangulate.h"

namespace GeoUtils {

Triangulate::Triangulate(const std::span<glm::vec2> &vertices)
    : mVertices(vertices) {
  execute();
}

float angleDiff(float angle0, float angle1) {
  auto diff = angle0 - angle1;
  if (diff >= glm::pi<float>() * 2.f) {
    diff -= glm::pi<float>() * 2.f;
  } else if (diff <= -glm::pi<float>() * 2.f) {
    diff += glm::pi<float>() * 2.f;
  }
  return diff;
}

float Triangulate::edgeAngle(int p0, int p1) {
  auto points = std::tuple{&mVertices[p0], &mVertices[p1]};
  auto xLen = std::get<0>(points)->x - std::get<1>(points)->x;
  auto yLen = std::get<0>(points)->y - std::get<1>(points)->y;
  if (yLen == 0.f && xLen != 0.f) {
    return 0.f;
  }
  return atan2(yLen, xLen);
}
void Triangulate::getOuterEdges() {

  mEdges.resize(mVertices.size());
  mPointAngles.resize(mVertices.size());

  mEdges[0] = {0, 1, edgeAngle(0, 1)};
  EdgeIdx *lastEdge = &mEdges[0];
  for (int i = 1; i < mVertices.size(); i++) {

    EdgeIdx &curEdge = mEdges[i];

    if (i + 1 != mVertices.size()) {
      curEdge = {.p0 = i, .p1 = i + 1};
    } else {
      curEdge = {.p0 = i, .p1 = 0};
    }

    curEdge.angle = edgeAngle(curEdge.p0, curEdge.p1);

    mPointAngles[i] = angleDiff(curEdge.angle, lastEdge->angle);
    lastEdge = &curEdge;
  }
  mPointAngles[0] = angleDiff(lastEdge->angle, mEdges[0].angle);
}
void Triangulate::execute() {
  getOuterEdges();
  // for (int i = 0; i < mVertices.size() - 2; i++) {

  //   TriIdx tri{i, i + 1, i + 2};
  // }
}
} // namespace GeoUtils