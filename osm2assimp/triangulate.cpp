#include "triangulate.h"
#include <format>
#include <iostream>

#include "CDT.h"

namespace GeoUtils {

Triangulate::Triangulate(const std::span<glm::vec2> &vertices)
    : mVertices(vertices) {

  execute();
}

void Triangulate::execute() {

  auto cdt = CDT::Triangulation<float>();

  cdt.insertVertices(
      mVertices.begin(), mVertices.end(),
      [](const glm::vec2 &p) { return p[0]; },
      [](const glm::vec2 &p) { return p[1]; });

  std::vector<CDT::Edge> edges;

  for (CDT::VertInd i = 0; i < mVertices.size() - 1; i++) {
    edges.push_back({i, i + 1});
  }
  edges.push_back({static_cast<CDT::VertInd>(mVertices.size() - 1), 0});

  cdt.insertEdges(edges);

  cdt.eraseOuterTriangles();

  mData.mFaces.resize(cdt.triangles.size());
  for (int i = 0; i < cdt.triangles.size(); i++) {
    auto &cdtTri = cdt.triangles[i];

    mData.mFaces[i] = {
        cdtTri.vertices[0],
        cdtTri.vertices[1],
        cdtTri.vertices[2],
    };
  }
  mData.mVertices.resize(cdt.vertices.size());
  for (int i = 0; i < cdt.vertices.size(); i++) {
    auto &cdtVert = cdt.vertices[i];
    mData.mVertices[i] = {cdtVert.x, cdtVert.y};
  }
}

} // namespace GeoUtils