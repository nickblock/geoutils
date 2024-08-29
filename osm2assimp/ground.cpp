#include "ground.h"
#include "assimp/mesh.h"
#include "delaunator.hpp"
#include "geometry.h"
#include "liminalspaces.h"
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

void Ground::addFootPrint(const Triangulate::Data &footprint, int type,
                          const std::string &name) {

  TVertIdx lastIdx = mGroundPoints.size();

  mGroundPoints.insert(mGroundPoints.end(), footprint.mVertices.begin(),
                       footprint.mVertices.end());

  if (type == OSMFeature::HIGHWAY) {
    if (!mLiminalSpaces) {
      mLiminalSpaces = std::make_unique<LiminalSpaces>(mBBox);
    }
    mLiminalSpaces->addIslands(footprint, name);
  }
}

void Ground::writeSvg(const std::filesystem::path &path) {

  if (mLiminalSpaces) {
    mLiminalSpaces->writeSvg(path);
  }
}

aiMesh *Ground::getMesh() {

  if (mLiminalSpaces) {

    mDataFlat = mLiminalSpaces->getInternalSpaces();

    auto geom3d = Geometry::extrude2dMesh(mDataFlat.mVertices, 0.f);

    return geom3d.toMesh();
  }
  return nullptr;
}
} // namespace GeoUtils