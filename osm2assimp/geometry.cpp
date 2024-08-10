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

bool lineIntersects2d(const Line &l0, const Line &l1, glm::vec2 *intersection) {
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

  if (intersection) {
    intersection->x = l0[0].x + (t * s1_x);
    intersection->y = l0[0].y + (t * s1_y);
  }
  if (s > 0 && s < 1 && t > 0 && t < 1) {
    return true;
  }

  return false; // No collision
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
    bool cross = true;
    cross = cross &&
            lineIntersects2d({this->mPoints[0], this->mPoints[3]},
                             {other.mPoints[0], other.mPoints[3]}, &result[0]);
    cross = cross &&
            lineIntersects2d({this->mPoints[1], this->mPoints[2]},
                             {other.mPoints[1], other.mPoints[2]}, &result[1]);

    if (cross) {
      return result;
    } else {
      return {this->mPoints[2], this->mPoints[3]};
    }
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
    throw std::runtime_error("Not enough nodes (<2), to crate line segment");
  }

  Geometry geometry;

  auto appendVertex = [&geometry](const glm::vec2 &point) {
    geometry.mData.mVertices.push_back(fromGround(point));
    geometry.mDataFlat.mVertices.push_back(point);
  };

  int numSegments = line.size() - 1;

  auto lastSeg = LineSegment(line[0], line[1], width);

  appendVertex(lastSeg.mPoints[1]);
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
  appendVertex(lastSeg.mPoints[0]);
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);

  glm::vec2 uvDistance(0.0f, 0.0f);
  geometry.mData.mTexCoords.push_back(
      {0.0f, uvDistance[0], static_cast<float>(featureId)});
  geometry.mData.mTexCoords.push_back(
      {1.0f, uvDistance[1], static_cast<float>(featureId)});

  for (int i = 1; i < numSegments; i++) {
    auto nextSeg = LineSegment(line[i + 0], line[i + 1], width);

    auto crossPoints = lastSeg.crossPoints(nextSeg);

    appendVertex(crossPoints[0]);
    throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
    appendVertex(crossPoints[1]);
    throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);

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

  appendVertex(lastSeg.mPoints[2]);
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
  appendVertex(lastSeg.mPoints[3]);
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);

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

    flatFace.insert(flatFace.begin(), face.begin(), face.end());

    faceIdx++;
  }

  return geometry;
}

Geometry Geometry::extrude2dMesh(const vector<glm::vec2> &in_vertices,
                                 float height, int featureId) {
  Geometry geometry;

  using Edge = std::pair<glm::vec2, glm::vec2>;
  using EdgeList = std::vector<Edge>;

  bool begin_eq_end = in_vertices[0] == in_vertices[in_vertices.size() - 1];

  geometry.mDataFlat.mVertices.insert(geometry.mDataFlat.mVertices.begin(),
                                      in_vertices.begin(), in_vertices.end());

  if (begin_eq_end) {
    geometry.mDataFlat.mVertices.pop_back();
  }

  if (geometry.mDataFlat.mVertices.size() < 3) {
    throw std::runtime_error("Not enough vertices (<3), to create a mesh");
  }

  size_t numBaseVertices = geometry.mDataFlat.mVertices.size();

  EdgeList edges;

  float lastEdgeAngle = 0;
  float accumEdge = 0;

  glm::vec2 center(0);

  for (size_t i = 0; i < numBaseVertices; i++) {

    auto &v1 = geometry.mDataFlat.mVertices[i];

    bool lastV = i + 1 == numBaseVertices;
    auto &v2 = lastV ? geometry.mDataFlat.mVertices[0]
                     : geometry.mDataFlat.mVertices[i + 1];

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
      auto tmp = geometry.mDataFlat.mVertices[i];
      geometry.mDataFlat.mVertices[i] =
          geometry.mDataFlat.mVertices[numBaseVertices - i - 1];
      geometry.mDataFlat.mVertices[numBaseVertices - i - 1] = tmp;
    }
  }

  bool doExtrude = height != 0.f;

  geometry.mData.mVertices.resize(doExtrude ? numBaseVertices * 6
                                            : numBaseVertices);
  geometry.mData.mNormals.resize(geometry.mData.mVertices.size());
  geometry.mData.mTexCoords.resize(
      texCoordScale != 0.0f ? geometry.mData.mVertices.size() : 0);
  geometry.mDataFlat.mVertices.resize(numBaseVertices);

  BBox bbox;

  for (size_t v = 0; v < numBaseVertices; v++) {
    const glm::vec2 &nv = geometry.mDataFlat.mVertices[v];

    // geometry.mDataFlat.mVertices[v] =

    geometry.mData.mVertices[v] = posFromLoc(nv.x, nv.y, 0.0);
    geometry.mData.mNormals[v] = -upNormal();

    bbox.add(geometry.mData.mVertices[v]);

    if (height > 0.f) {
      geometry.mData.mVertices[v + numBaseVertices] =
          posFromLoc(nv.x, nv.y, height);
      bbox.add(geometry.mData.mVertices[v + numBaseVertices]);
      geometry.mData.mNormals[v + numBaseVertices] = upNormal();
    }
  }

  geometry.mData.mFaces.resize(height > 0.f ? 2 + numBaseVertices : 1);
  geometry.mData.mFaces[0].resize(numBaseVertices);

  geometry.mDataFlat.mFaces.resize(1);
  geometry.mDataFlat.mFaces[0].resize(numBaseVertices);

  for (size_t i = 0; i < numBaseVertices; i++) {
    geometry.mData.mFaces[0][i] = numBaseVertices - i - 1;
    geometry.mDataFlat.mFaces[0][i] = numBaseVertices - i - 1;
  }

  if (doExtrude) {

    geometry.mData.mFaces[1].resize(numBaseVertices);

    for (size_t i = 0; i < numBaseVertices; i++) {
      geometry.mData.mFaces[1][i] = numBaseVertices + i;
    }

    for (int f = 0; f < numBaseVertices; f++) {

      int fn = f;
      if (f + 1 == numBaseVertices)
        fn = -1;

      int index = numBaseVertices * 2 + 4 * f;
      glm::vec3 *corners = &geometry.mData.mVertices[index];

      corners[3] = geometry.mData.mVertices[fn + 1];
      corners[2] = geometry.mData.mVertices[f + 0];
      corners[1] = geometry.mData.mVertices[f + numBaseVertices + 0];
      corners[0] = geometry.mData.mVertices[fn + numBaseVertices + 1];
      glm::vec3 v1 = corners[1] - corners[0];
      glm::vec3 v2 = corners[2] - corners[0];
      glm::vec3 n = glm::normalize(glm::cross(v1, v2));

      if (!zUp) {
        n = -n;
      }

      if (isnan(n)) {
        throw std::runtime_error("Normal calc failed!");
      }

      glm::vec3 *vNormals = &geometry.mData.mNormals[index];
      vNormals[0] = n;
      vNormals[1] = n;
      vNormals[2] = n;
      vNormals[3] = n;

      if (geometry.mData.mTexCoords.size()) {
        glm::vec3 *texCoord = &geometry.mData.mTexCoords[index];
        float width = glm::distance(corners[0], corners[1]);
        float texCoordU = std::round(width / texCoordScale);
        float texCoordV = std::round(height / texCoordScale);

        texCoord[0] = {texCoordU, texCoordV, static_cast<float>(featureId)};
        texCoord[1] = {0.f, texCoordV, static_cast<float>(featureId)};
        texCoord[2] = {0.f, 0.f, static_cast<float>(featureId)};
        texCoord[3] = {texCoordU, 0.f, static_cast<float>(featureId)};
      }

      Face &face = geometry.mData.mFaces[2 + f];
      face.resize(4);

      face[0] = index + 0;
      face[1] = index + 1;
      face[2] = index + 2;
      face[3] = index + 3;
    }
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

Geometry::DataFlat Geometry::triangulate(const std::span<glm::vec2> &vertices) {

  // TODO avoid copy / check RVO
  return Triangulate(vertices).getData();
}

void Geometry::DataFlat::writeSvg(const std::filesystem::path &filepath) {

  constexpr float kPrecision = 1e3;

  std::ofstream file = std::ofstream(filepath);

  glm::vec2 min{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
  glm::vec2 max{std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min()};

  for (auto &p : mVertices) {
    min.x = std::min(p.x * kPrecision, min.x);
    min.y = std::min(p.y * kPrecision, min.y);
    max.x = std::max(p.x * kPrecision, max.x);
    max.y = std::max(p.y * kPrecision, max.y);
  }

  min.x -= 1;
  min.y -= 1;
  max.x += 1;
  max.y += 1;

  file << std::format("<svg viewBox=\"{} {} {} {}\" xmlns="
                      "\"http://www.w3.org/2000/svg\">",
                      0, 0, (max.x - min.x), (max.y - min.y))
       << std::endl;

  for (auto &face : mFaces) {

    file << "<polygon points=\"";
    for (auto &idx : face) {
      auto &p = mVertices[idx];
      file << std::format("{},{} ", (p.x * kPrecision - min.x),
                          (p.y * kPrecision - min.y))
           << std::endl;
    }
    file << "\" fill=\"white\" stroke=\"red\" />" << std::endl;
  }

  file << "</svg>" << std::endl;
}

} // namespace GeoUtils