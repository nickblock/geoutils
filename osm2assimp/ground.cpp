#include "ground.h"
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

inline std::size_t Ground::hashKey(Point point) const {
  const double dx = point.first - mExtents[0].x;
  const double dy = point.second - mExtents[0].x;
  return delaunator::fast_mod(
      static_cast<std::size_t>(std::llround(std::floor(
          delaunator::pseudo_angle(dx, dy) * static_cast<double>(kHashSize)))),
      kHashSize);
}

void Ground::addFootPrint(const std::vector<double> &points, int type) {
  mGroundPoints.insert(mGroundPoints.end(), points.begin(), points.end());

  for (int i = 0; i < points.size(); i += 2) {

    mPointTypes[hashKey({points[i], points[i + 1]})] = type;
  }
}

std::vector<double>
Ground::findGroundTris(const std::vector<double> &delaunayTris) {
  std::vector<double> groundTris;

  for (int i = 0; i < delaunayTris.size(); i += 6) {

    auto p0 = Point{delaunayTris[i + 0], delaunayTris[i + 1]};
    auto p1 = Point{delaunayTris[i + 2], delaunayTris[i + 3]};
    auto p2 = Point{delaunayTris[i + 4], delaunayTris[i + 5]};

    if (mPointTypes[hashKey(p0)] ==
        mPointTypes[hashKey(p1) == mPointTypes[hashKey(p2)]]) {
    }
  }

  return {0};
}

// std::vector<p2t::Point *> fromGLM(const std::vector<glm::vec2> points) {
//   std::vector<p2t::Point *> result(points.size());

//   for (int i = 0; i < points.size(); i++) {
//     result[i] = new p2t::Point(points[i].x, points[i].y);
//   }
//   return result;
// }

// void freePointList(std::vector<p2t::Point *> points) {
//   for (auto p : points) {
//     delete p;
//   }
// }

stringstream pointsss(const std::vector<glm::vec2> &points,
                      const glm::vec2 &min, float scale) {
  stringstream ss;

  ss << "<polygon points=\"";

  for (auto &p : points) {
    ss << std::format("%d,%d ", (p.x - min.x) * scale, (p.y - min.y) * scale);
  }

  ss << "\" fill=\"none\" stroke=\"white\" />";

  return ss;
}

void Ground::writeSvg(const std::filesystem::path &path, float scale) {

  ofstream file = ofstream(path);

  file << std::format("<svg viewBox=\"%d %d %d %d\" xmlns="
                      "\"http://www.w3.org/2000/svg\">",
                      0, 0, (mBBox.mMax.x - mBBox.mMin.x) * scale,
                      (mBBox.mMax.y - mBBox.mMin.y) * scale)
       << endl;

  // for (auto &iter : mSubtractions) {
  //   file << pointsss(std::get<Poly>(iter), mBBox.mMin, scale).str() << endl;
  // }

  file << "</svg>" << endl;
}

aiMesh *Ground::getMesh() {

  writeSvg("/tmp/ground.svg", 10.f);

  float extra = 1.f;
  std::vector<glm::vec2> boxPoints = {
      {mBBox.mMin.x - extra, mBBox.mMin.y - extra},
      {mBBox.mMin.x - extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMin.y - extra}};

  delaunator::Delaunator delaunator(mGroundPoints);

  aiMesh *mesh = new aiMesh();

  mesh->mNumVertices = delaunator.triangles.size();
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  mesh->mNumUVComponents[0] = 2;

  mesh->mNumFaces = delaunator.triangles.size() / 3;
  mesh->mFaces = new aiFace[delaunator.triangles.size() / 3];

  int vertexIdx = 0;
  auto upNormal = Geometry::upNormal();
  int faceIdx = 0;
  for (size_t i = 0; i < delaunator.triangles.size(); i += 3) {

    auto &face = mesh->mFaces[faceIdx];

    face.mNumIndices = 3;
    face.mIndices = new unsigned int[face.mNumIndices];

    for (size_t j = 0; j < 3; j++) {
      face.mIndices[j] = vertexIdx;

      glm::vec2 point = {
          delaunator.coords[2 * delaunator.triangles[i + j]],
          delaunator.coords[2 * delaunator.triangles[i + j] + 1]};

      glm::vec3 vertex = Geometry::posFromLoc(point.x, point.y, 0.f);
      glm::vec3 uv = mBBox.fraction({point.x, point.y, 0.0});

      mesh->mVertices[vertexIdx] = {vertex.x, vertex.y, vertex.z};
      mesh->mNormals[vertexIdx] = {upNormal.x, upNormal.y, upNormal.z};
      mesh->mTextureCoords[0][vertexIdx] = {uv.x, uv.y, uv.z};

      vertexIdx++;
    }
    faceIdx++;
  }

  return mesh;
}
} // namespace GeoUtils