#include "ground.h"
#include "CDT.h"
#include "assimp/mesh.h"
#include "delaunator.hpp"
#include "geometry.h"
#include <fstream>
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::ofstream;
using std::stringstream;

namespace GeoUtils {
Ground::Ground(const std::vector<glm::vec2> &extents) : mExtents(extents) {
  for (auto &p : extents) {
    mBBox.add(glm::vec3(p, 0.0f));
  }
}

void Ground::addFootPrint(const Triangulate::Data &footprint) {

  TVertIdx lastIdx = mGroundPoints.size();

  mGroundPoints.insert(mGroundPoints.end(), footprint.mVertices.begin(),
                       footprint.mVertices.end());
}

void Ground::writeSvg(const std::filesystem::path &path) {

  mDataFlat.writeSvg(path);
}

aiMesh *Ground::getMesh() {

  auto cdt = CDT::Triangulation<float>(
      CDT::VertexInsertionOrder::Auto,
      CDT::IntersectingConstraintEdges::TryResolve, 0.1f);

  float extra = 1.f;
  std::vector<glm::vec2> boxPoints = {
      {mBBox.mMin.x - extra, mBBox.mMin.y - extra},
      {mBBox.mMin.x - extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMin.y - extra}};

  cdt.insertVertices(
      mGroundPoints.begin(), mGroundPoints.end(),
      [](const glm::vec2 &p) { return p[0]; },
      [](const glm::vec2 &p) { return p[1]; });

  cdt.insertEdges(
      mEdges.begin(), mEdges.end(), [](const Edge &edge) { return edge[0]; },
      [](const Edge &edge) { return edge[1]; });

  cdt.eraseOuterTriangles();

  mDataFlat.mFaces.resize(cdt.triangles.size());

  for (int i = 0; i < cdt.triangles.size(); i++) {
    auto &cdtTri = cdt.triangles[i];

    auto &tri = mDataFlat.mFaces[i];
    tri.resize(3);

    tri[0] = cdtTri.vertices[0];
    tri[1] = cdtTri.vertices[1];
    tri[2] = cdtTri.vertices[2];
  }

  mDataFlat.mVertices.resize(cdt.vertices.size());

  for (int i = 0; i < cdt.vertices.size(); i++) {
    auto &p = mDataFlat.mVertices[i];
    p.x = cdt.vertices[i].x;
    p.y = cdt.vertices[i].y;
  }

  aiMesh *mesh = new aiMesh();

  mesh->mNumVertices = cdt.vertices.size();
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  mesh->mNumUVComponents[0] = 2;

  mesh->mNumFaces = cdt.triangles.size();
  mesh->mFaces = new aiFace[cdt.triangles.size()];

  auto upNormal = Geometry::upNormal();
  for (size_t i = 0; i < cdt.triangles.size(); i++) {

    auto &face = mesh->mFaces[i];

    face.mNumIndices = 3;
    face.mIndices = new unsigned int[face.mNumIndices];

    auto &cdtTri = cdt.triangles[i];

    for (size_t j = 0; j < 3; j++) {

      auto cdtIdx = cdtTri.vertices[j];
      auto &cdtVertex = cdt.vertices[cdtIdx];

      face.mIndices[j] = cdtIdx;

      glm::vec2 point = {cdtVertex.x, cdtVertex.y};

      glm::vec3 vertex = Geometry::posFromLoc(point.x, point.y, 0.f);
      glm::vec3 uv = mBBox.fraction({point.x, point.y, 0.0});

      mesh->mVertices[cdtIdx] = {vertex.x, vertex.y, vertex.z};
      mesh->mNormals[cdtIdx] = {upNormal.x, upNormal.y, upNormal.z};
      mesh->mTextureCoords[0][cdtIdx] = {uv.x, uv.y, uv.z};
    }
  }

  return mesh;
}
} // namespace GeoUtils