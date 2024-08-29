#include "geometry.h"
#include "assimp/scene.h"
#include "common.h"
#include "glm/gtc/constants.hpp"
#include "triangulate.h"
#include "utils.h"
#include <array>
#include <format>
#include <fstream>
#include <set>

using std::vector;

namespace GeoUtils {

bool Geometry::zUp = false;
float Geometry::texCoordScale = 0.0f;

bool pointOnLine(const Line &line, const glm::vec2 &point) {
  float line_x = line[1].x - line[0].x;
  float line_y = line[1].y - line[0].y;

  float point_x = point.x - line[0].x;
  float point_y = point.y - line[0].y;

  float slopeLine = line_y / line_x;
  float slopePoint = point_y / point_x;

  if (abs(slopeLine - slopePoint) > glm::epsilon<float>()) {
    return false;
  }

  float minX = std::min(line[0].x, line[1].x);
  float maxX = std::max(line[0].x, line[1].x);
  float minY = std::min(line[0].y, line[1].y);
  float maxY = std::max(line[0].y, line[1].y);

  return (point.x >= minX && point.x <= maxX) &&
         (point.y >= minY && point.y <= maxY);
}

std::tuple<glm::vec2, bool> lineIntersects2d(const Line &l0, const Line &l1) {
  float s1_x, s1_y, s2_x, s2_y;
  s1_x = l0[1].x - l0[0].x;
  s1_y = l0[1].y - l0[0].y;
  s2_x = l1[1].x - l1[0].x;
  s2_y = l1[1].y - l1[0].y;

  float s, t;
  s = (-s1_y * (l0[0].x - l1[0].x) + s1_x * (l0[0].y - l1[0].y)) /
      (-s2_x * s1_y + s1_x * s2_y);
  t = (s2_x * (l0[0].y - l1[0].y) - s2_y * (l0[0].x - l1[0].x)) /
      (-s2_x * s1_y + s1_x * s2_y);

  glm::vec2 intersection;
  intersection.y = l0[0].y + (t * s1_y);
  intersection.x = l0[0].x + (t * s1_x);
  if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
    return {intersection, true};
  }

  return {intersection, false};
}

glm::vec3 Geometry::upNormal() {
  if (zUp) {
    return glm::vec3(0.f, 0.f, 1.f);
  } else {
    return glm::vec3(0.f, 1.f, 0.f);
  }
}
glm::vec3 Geometry::posFromLoc(double lon, double lat, double height) {
  if (zUp) {
    return glm::vec3(lon, lat, height);
  } else {
    return glm::vec3(-lon, height, lat);
  }
}

glm::vec3 Geometry::fromGround(const glm::vec2 &groundCoords) {
  if (zUp) {
    return glm::vec3(groundCoords, 0.0f);
  } else {
    return {-groundCoords.x, 0.0f, groundCoords.y};
  }
}

struct LineSegment {
  LineSegment(const glm::vec2 &p0, const glm::vec2 &p1, float width) {
    auto dir = glm::normalize(p1 - p0);

    auto norm = glm::vec2(-dir[1], dir[0]);
    auto normWidth = norm * width / 2.0f;

    mArcTan = atan2(dir.y, dir.x);

    mPoints = {
        p0 + normWidth,
        p0 - normWidth,
        p1 - normWidth,
        p1 + normWidth,
    };
  }
  std::array<glm::vec2, 2> crossPoints(const LineSegment &other) {
    std::array<glm::vec2, 2> result;

    if (fabs(mArcTan - other.mArcTan) < glm::epsilon<float>()) {
      // parallel
      return {this->mPoints[3], this->mPoints[2]};
    }
    bool cross = true;

    auto result0 = lineIntersects2d({this->mPoints[0], this->mPoints[3]},
                                    {other.mPoints[0], other.mPoints[3]});

    auto result1 = lineIntersects2d({this->mPoints[1], this->mPoints[2]},
                                    {other.mPoints[1], other.mPoints[2]});

    return {std::get<glm::vec2>(result0), std::get<glm::vec2>(result1)};
  }

  std::array<glm::vec2, 4> mPoints;
  float mArcTan;
};

bool isnan(const glm::vec3 &v) {
  return glm::isnan(v.x) || glm::isnan(v.y) || glm::isnan(v.z);
}

void throw_if_nan(const glm::vec3 &v) {
  if (isnan(v)) {
    throw std::runtime_error("vec is nan in geomconversion");
  }
}

Geometry Geometry::meshFromLine(const std::vector<glm::vec2> &line, float width,
                                int featureId) {

  if (line.size() < 2) {
    throw std::runtime_error("Not enough nodes (<2), to create line segment");
  }

  Geometry geometry;

  std::vector<glm::vec2> side0;
  std::vector<glm::vec2> side1;

  auto appendVertex = [&geometry, &side0, &side1](const glm::vec2 &point) {
    geometry.mData.mVertices.push_back(fromGround(point));

    throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);

    static bool appendSide0 = true;
    appendSide0 ? side0.push_back(point) : side1.push_back(point);
    appendSide0 = !appendSide0;
  };

  int numSegments = line.size() - 1;

  auto lastSeg = LineSegment(line[0], line[1], width);

  appendVertex(lastSeg.mPoints[0]);
  appendVertex(lastSeg.mPoints[1]);

  glm::vec2 uvDistance(0.0f, 0.0f);
  geometry.mData.mTexCoords.push_back(
      {0.0f, uvDistance[0], static_cast<float>(featureId)});
  geometry.mData.mTexCoords.push_back(
      {1.0f, uvDistance[1], static_cast<float>(featureId)});

  for (int i = 1; i < numSegments; i++) {
    auto nextSeg = LineSegment(line[i + 0], line[i + 1], width);

    auto crossPoints = lastSeg.crossPoints(nextSeg);

    appendVertex(crossPoints[0]);
    appendVertex(crossPoints[1]);

    uvDistance += glm::vec2{
        glm::distance(geometry.mData.mVertices[i * 2 + 0],
                      geometry.mData.mVertices[i * 2 - 2]) /
            width,
        glm::distance(geometry.mData.mVertices[i * 2 + 1],
                      geometry.mData.mVertices[i * 2 - 1]) /
            width,
    };

    geometry.mData.mTexCoords.push_back(
        {0.0f, uvDistance[0], static_cast<float>(featureId)});
    geometry.mData.mTexCoords.push_back(
        {1.0f, uvDistance[1], static_cast<float>(featureId)});

    lastSeg = nextSeg;
  }

  // if (isSame(line[0], line[line.size() - 1])) {

  //   auto crossPoints =
  //       lastSeg.crossPoints(LineSegment(line[0], line[1], width));

  //   appendVertex(crossPoints[0]);
  //   appendVertex(crossPoints[1]);
  //   geometry.mData.mVertices[0] =
  //       geometry.mData.mVertices[geometry.mData.mVertices.size() - 2];
  //   geometry.mData.mVertices[1] =
  //       geometry.mData.mVertices[geometry.mData.mVertices.size() - 1];

  //   side0[0] = crossPoints[0];
  //   side1[0] = crossPoints[1];
  // } else {
  appendVertex(lastSeg.mPoints[3]);
  appendVertex(lastSeg.mPoints[2]);
  // }

  uvDistance += glm::vec2{
      glm::distance(
          geometry.mData.mVertices[geometry.mData.mVertices.size() - 1],
          geometry.mData.mVertices[geometry.mData.mVertices.size() - 3]) /
          width,
      glm::distance(
          geometry.mData.mVertices[geometry.mData.mVertices.size() - 2],
          geometry.mData.mVertices[geometry.mData.mVertices.size() - 4]) /
          width,
  };

  geometry.mData.mTexCoords.push_back(
      {0.0f, uvDistance[0], static_cast<float>(featureId)});
  geometry.mData.mTexCoords.push_back(
      {1.0f, uvDistance[1], static_cast<float>(featureId)});

  geometry.mData.mNormals.resize(geometry.mData.mVertices.size());
  for (int i = 0; i < geometry.mData.mVertices.size(); i++) {
    geometry.mData.mNormals[i] = upNormal();
  }

  geometry.mData.mFaces.resize(numSegments);
  geometry.mDataFlat.mFaces.resize(numSegments);

  int vertIdx = 0;
  int faceIdx = 0;

  for (int i = 0; i < numSegments; i++) {
    auto &face = geometry.mData.mFaces[faceIdx];
    auto &flatFace = geometry.mDataFlat.mFaces[faceIdx];

    face.resize(4);
    face[0] = (i * 2) + 0;
    face[1] = (i * 2) + 1;
    face[2] = (i * 2) + 3;
    face[3] = (i * 2) + 2;

    faceIdx++;
  }

  geometry.mDataFlat.mVertices.insert(geometry.mDataFlat.mVertices.begin(),
                                      side1.begin(), side1.end());

  std::reverse(side0.begin(), side0.end());
  geometry.mDataFlat.mVertices.insert(geometry.mDataFlat.mVertices.end(),
                                      side0.begin(), side0.end());

  geometry.mDataFlat.mFaces.resize(1);
  geometry.mDataFlat.mFaces[0].resize(geometry.mDataFlat.mVertices.size());
  for (int i = 0; i < geometry.mDataFlat.mVertices.size(); i++) {
    geometry.mDataFlat.mFaces[0][i] = i;
  }

  return geometry;
}

Geometry Geometry::extrude2dMesh(const vector<glm::vec2> &in_vertices,
                                 float height, int featureId) {
  Geometry geometry;

  geometry.mDataFlat.mVertices.insert(geometry.mDataFlat.mVertices.begin(),
                                      in_vertices.begin(), in_vertices.end());

  geometry.mFeatureId = featureId;
  geometry.mData = geometry.mDataFlat.extrude3DFromFlat(height, featureId);

  return geometry;
}
Geometry::Data3D Geometry::DataFlat::extrude3DFromFlat(float height,
                                                       int featureId) {

  using Edge = std::pair<glm::vec2, glm::vec2>;
  using EdgeList = std::vector<Edge>;

  Data3D data3d;

  bool begin_eq_end = mVertices[0] == mVertices[mVertices.size() - 1];

  if (begin_eq_end) {
    mVertices.pop_back();
  }

  if (mVertices.size() < 3) {
    throw std::runtime_error("Not enough vertices (<3), to create a mesh");
  }

  size_t numBaseVertices = mVertices.size();

  EdgeList edges;

  float lastEdgeAngle = 0;
  float accumEdge = 0;

  glm::vec2 center(0);

  for (size_t i = 0; i < numBaseVertices; i++) {

    auto &v1 = mVertices[i];

    bool lastV = i + 1 == numBaseVertices;
    auto &v2 = lastV ? mVertices[0] : mVertices[i + 1];

    center += v1;

    Edge newEdge(v1, v2);

    float edgeAngle = atan2(v2.x - v1.x, v2.y - v1.y);

    if (i != 0) {
      float edgeDiff = edgeAngle - lastEdgeAngle;
      if (edgeDiff > glm::pi<double>()) {
        edgeDiff -= glm::pi<double>();
      }
      if (edgeDiff < -glm::pi<double>()) {
        edgeDiff += glm::pi<double>();
      }
      accumEdge += edgeDiff;
    }
    lastEdgeAngle = edgeAngle;

    // int edgeNum = edges.size();

    // for(int e=0; e<edgeNum-1; e++) {

    //   if(lastV && e == 0) continue;

    //   Edge other = edges[e];
    //   glm::vec3 intersect;
    //   if(lineIntersects2d(other.first.x, other.first.y, other.second.x,
    //   other.second.y,
    //     newEdge.first.x, newEdge.first.y, newEdge.second.x,
    //     newEdge.second.y, &intersect)) {

    //     return nullptr;
    //   }
    // }

    edges.push_back(Edge(v1, v2));
  }

  if (accumEdge > 0.0) {

    for (size_t i = 0; i < numBaseVertices / 2; i++) {
      auto tmp = mVertices[i];
      mVertices[i] = mVertices[numBaseVertices - i - 1];
      mVertices[numBaseVertices - i - 1] = tmp;
    }
  }

  bool doExtrude = height != 0.f;

  data3d.mVertices.resize(doExtrude ? numBaseVertices * 6 : numBaseVertices);
  data3d.mNormals.resize(data3d.mVertices.size());
  data3d.mTexCoords.resize(texCoordScale != 0.0f ? data3d.mVertices.size() : 0);
  mVertices.resize(numBaseVertices);

  BBox bbox;

  for (size_t v = 0; v < numBaseVertices; v++) {
    const glm::vec2 &nv = mVertices[v];

    // mVertices[v] =

    data3d.mVertices[v] = posFromLoc(nv.x, nv.y, 0.0);
    data3d.mNormals[v] = -upNormal();

    bbox.add(data3d.mVertices[v]);

    if (height > 0.f) {
      data3d.mVertices[v + numBaseVertices] = posFromLoc(nv.x, nv.y, height);
      bbox.add(data3d.mVertices[v + numBaseVertices]);
      data3d.mNormals[v + numBaseVertices] = upNormal();
    }
  }

  data3d.mFaces.resize(height > 0.f ? 2 + numBaseVertices : 1);
  data3d.mFaces[0].resize(numBaseVertices);

  mFaces.resize(1);
  mFaces[0].resize(numBaseVertices);

  for (size_t i = 0; i < numBaseVertices; i++) {
    data3d.mFaces[0][i] = numBaseVertices - i - 1;
    mFaces[0][i] = numBaseVertices - i - 1;
  }

  if (doExtrude) {

    data3d.mFaces[1].resize(numBaseVertices);

    for (size_t i = 0; i < numBaseVertices; i++) {
      data3d.mFaces[1][i] = numBaseVertices + i;
    }

    for (int f = 0; f < numBaseVertices; f++) {

      int fn = f;
      if (f + 1 == numBaseVertices)
        fn = -1;

      int index = numBaseVertices * 2 + 4 * f;
      glm::vec3 *corners = &data3d.mVertices[index];

      corners[3] = data3d.mVertices[fn + 1];
      corners[2] = data3d.mVertices[f + 0];
      corners[1] = data3d.mVertices[f + numBaseVertices + 0];
      corners[0] = data3d.mVertices[fn + numBaseVertices + 1];
      glm::vec3 v1 = corners[1] - corners[0];
      glm::vec3 v2 = corners[2] - corners[0];
      glm::vec3 n = glm::normalize(glm::cross(v1, v2));

      if (!zUp) {
        n = -n;
      }

      if (isnan(n)) {
        throw std::runtime_error("Normal calc failed!");
      }

      glm::vec3 *vNormals = &data3d.mNormals[index];
      vNormals[0] = n;
      vNormals[1] = n;
      vNormals[2] = n;
      vNormals[3] = n;

      if (data3d.mTexCoords.size()) {
        glm::vec3 *texCoord = &data3d.mTexCoords[index];
        float width = glm::distance(corners[0], corners[1]);
        float texCoordU = std::round(width / texCoordScale);
        float texCoordV = std::round(height / texCoordScale);

        texCoord[0] = {texCoordU, texCoordV, static_cast<float>(featureId)};
        texCoord[1] = {0.f, texCoordV, static_cast<float>(featureId)};
        texCoord[2] = {0.f, 0.f, static_cast<float>(featureId)};
        texCoord[3] = {texCoordU, 0.f, static_cast<float>(featureId)};
      }

      Face &face = data3d.mFaces[2 + f];
      face.resize(4);

      face[0] = index + 0;
      face[1] = index + 1;
      face[2] = index + 2;
      face[3] = index + 3;
    }
  }
  return data3d;
}
Geometry Geometry::meshFromJunction(const glm::vec2 &center,
                                    const std::vector<glm::vec2> &offroads,
                                    float width) {
  Geometry geometry;

  geometry.mDataFlat.mVertices.resize(offroads.size() * 3);
  geometry.mDataFlat.mFaces.resize(1);
  geometry.mDataFlat.mFaces[0].resize(geometry.mDataFlat.mVertices.size());

  struct Spoke {
    std::array<glm::vec2, 2> bar;
    glm::vec2 dir;

    Line rightArmBack() { return {bar[1], bar[1] + dir}; }
    Line leftArmBack() { return {bar[0], bar[0] + dir}; }
  };

  std::vector<Spoke> spokes(offroads.size());

  for (int i = 0; i < offroads.size(); i++) {
    auto &spoke = spokes[i];
    spoke.dir = center - offroads[i];
    auto dirN = glm::normalize(spoke.dir);

    auto cross = glm::vec2(-dirN[1], dirN[0]);
    auto crossBar = cross * width * 0.5f;

    spoke.bar = {offroads[i] - crossBar, offroads[i] + crossBar};
  }

  std::vector<glm::vec2> inBetweenSpokes(offroads.size());

  for (int i = 0; i < offroads.size(); i++) {
    auto j = i + 1;
    if (j == offroads.size()) {
      j = 0;
    }
    auto &spoke0 = spokes[i];
    auto &spoke1 = spokes[j];

    auto intersectionResult =
        lineIntersects2d(spoke0.rightArmBack(), spoke1.leftArmBack());

    inBetweenSpokes[i] = std::get<glm::vec2>(intersectionResult);

    // check spoke
    auto r = Triangulate::reflexPoint(inBetweenSpokes[i], spoke0.bar[1],
                                      spoke0.bar[0]);
    assert(r > 0);

    auto vertIdx = i * 3;

    geometry.mDataFlat.mVertices[vertIdx + 0] = spoke0.bar[0];
    geometry.mDataFlat.mVertices[vertIdx + 1] = spoke0.bar[1];
    geometry.mDataFlat.mVertices[vertIdx + 2] = inBetweenSpokes[i];

    geometry.mDataFlat.mFaces[0][vertIdx + 0] = vertIdx + 0;
    geometry.mDataFlat.mFaces[0][vertIdx + 1] = vertIdx + 1;
    geometry.mDataFlat.mFaces[0][vertIdx + 2] = vertIdx + 2;
  }

  return geometry;
}

aiMesh *Geometry::Data3D::toMesh() const {
  aiMesh *newMesh = new aiMesh;
  newMesh->mNumVertices = mVertices.size();
  newMesh->mVertices = new aiVector3D[mVertices.size()];
  newMesh->mNormals = new aiVector3D[mVertices.size()];

  memcpy(newMesh->mVertices, mVertices.data(),
         mVertices.size() * sizeof(glm::vec3));
  memcpy(newMesh->mNormals, mNormals.data(),
         mNormals.size() * sizeof(glm::vec3));

  if (mTexCoords.size()) {
    newMesh->mNumUVComponents[0] = 2;
    newMesh->mTextureCoords[0] = new aiVector3D[mVertices.size()];
    memcpy(newMesh->mTextureCoords[0], mTexCoords.data(),
           mVertices.size() * sizeof(glm::vec3));
  }

  int numFaces = mFaces.size();
  newMesh->mNumFaces = numFaces;
  newMesh->mFaces = new aiFace[numFaces];
  for (int i = 0; i < numFaces; i++) {
    aiFace &face = newMesh->mFaces[i];
    face.mNumIndices = mFaces[i].size();
    face.mIndices = new unsigned int[face.mNumIndices];
    memcpy(face.mIndices, mFaces[i].data(),
           face.mNumIndices * sizeof(unsigned int));
  }

  return newMesh;
}

Geometry::DataFlat &
Geometry::DataFlat::operator+(const Geometry::DataFlat &otherData) {

  TVertIdx lastIdx = mVertices.size();

  mVertices.insert(mVertices.end(), otherData.mVertices.begin(),
                   otherData.mVertices.end());
  Face face(otherData.mVertices.size());
  for (int i = 0; i < otherData.mVertices.size(); i++) {
    face[i] = lastIdx + i;
  }
  mFaces.emplace_back(face);

  return *this;
}

} // namespace GeoUtils