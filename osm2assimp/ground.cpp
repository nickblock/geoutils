#include "ground.h"
#include "CDT.h"
#include "assimp/mesh.h"
#include "delaunator.hpp"
#include "geometry.h"
#include "roadnetwork.h"
#include "svg.h"
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

Ground::~Ground() = default;

void Ground::addFootPrint(const Triangulate::Data &footprint,
                          GroundTypes type) {

  TVertIdx lastIdx = mGroundPoints.size();

  mGroundPoints.insert(mGroundPoints.end(), footprint.mVertices.begin(),
                       footprint.mVertices.end());

  if (type == Road) {
    if (!mRoadNetwork) {
      mRoadNetwork = std::make_unique<RoadNetwork>(mBBox);
    }
    mRoadNetwork->addRoad(footprint);
  }
}

void Ground::writeSvg(const std::filesystem::path &path) {

  auto svg = SVGWriter();

  svg.addPolygons(mDataFlat);

  svg.write(path);
}

aiMesh *Ground::getMesh() {

  if (mRoadNetwork) {

    mDataFlat = mRoadNetwork->getInternalSpaces();
  }

  aiMesh *mesh = new aiMesh();

  // mesh->mNumVertices = mDataFlat.mVertices.size();
  // mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  // mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  // mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  // mesh->mNumUVComponents[0] = 2;

  // mesh->mNumFaces = mDataFlat.mFaces.size();
  // mesh->mFaces = new aiFace[mDataFlat.mFaces.size()];

  // auto upNormal = Geometry::upNormal();
  // for (size_t i = 0; i < mDataFlat.mFaces.size(); i++) {

  //   auto &face = mesh->mFaces[i];
  //   auto &dataFace = mDataFlat.mFaces[i];

  //   face.mNumIndices = dataFace.size();
  //   face.mIndices = new unsigned int[face.mNumIndices];

  //   for (size_t j = 0; j < dataFace.size(); j++) {

  //     auto cdtIdx = dataFace[j];
  //     auto &cdtVertex = mDataFlat[cdtIdx];

  //     face.mIndices[j] = cdtIdx;

  //     glm::vec2 point = {cdtVertex.x, cdtVertex.y};

  //     glm::vec3 vertex = Geometry::posFromLoc(point.x, point.y, 0.f);
  //     glm::vec3 uv = mBBox.fraction({point.x, point.y, 0.0});

  //     mesh->mVertices[cdtIdx] = {vertex.x, vertex.y, vertex.z};
  //     mesh->mNormals[cdtIdx] = {upNormal.x, upNormal.y, upNormal.z};
  //     mesh->mTextureCoords[0][cdtIdx] = {uv.x, uv.y, uv.z};
  //   }
  // }

  return mesh;
}
} // namespace GeoUtils