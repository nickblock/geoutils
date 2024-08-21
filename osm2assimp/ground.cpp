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

  if (mRoadNetwork) {
    mRoadNetwork->writeSvg(path);
  }
}

aiMesh *Ground::getMesh() {

  if (mRoadNetwork) {

    mDataFlat = mRoadNetwork->getInternalSpaces();
  }

  aiMesh *mesh = new aiMesh();

  mesh->mNumVertices = mDataFlat.mVertices.size();
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];

  mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  mesh->mNumUVComponents[0] = 2;

  mesh->mNumFaces = mDataFlat.mFaces.size();
  mesh->mFaces = new aiFace[mDataFlat.mFaces.size()];

  auto upNormal = Geometry::upNormal();
  for (int i = 0; i < mesh->mNumVertices; i++) {
    auto vec3 = Geometry::fromGround(mDataFlat.mVertices[i]);
    mesh->mVertices[i] = {vec3.x, vec3.y, vec3.z};

    mesh->mNormals[i] = {upNormal.x, upNormal.y, upNormal.z};
    mesh->mTextureCoords[0][i] = {mDataFlat.mVertices[i].x,
                                  mDataFlat.mVertices[i].y, 0.0f};
  }

  for (size_t i = 0; i < mDataFlat.mFaces.size(); i++) {

    auto &face = mesh->mFaces[i];
    auto &dataFace = mDataFlat.mFaces[i];

    face.mNumIndices = dataFace.size();
    face.mIndices = new unsigned int[face.mNumIndices];

    for (size_t j = 0; j < dataFace.size(); j++) {
      face.mIndices[j] = dataFace[j];
    }
  }

  return mesh;
}
} // namespace GeoUtils