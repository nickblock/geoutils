#include "geometry.h"
#include "common.h"
#include "glm/gtc/constants.hpp"
#include "utils.h"
#include <array>

using std::vector;

namespace GeoUtils {

bool Geometry::zUp = false;
float Geometry::texCoordScale = 0.0f;

bool lineIntersects2d(float p0_x, float p0_y, float p1_x, float p1_y,
                      float p2_x, float p2_y, float p3_x, float p3_y,
                      glm::vec2 *intersection) {
  float s1_x, s1_y, s2_x, s2_y;
  s1_x = p1_x - p0_x;
  s1_y = p1_y - p0_y;
  s2_x = p3_x - p2_x;
  s2_y = p3_y - p2_y;

  float s, t;
  s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) /
      (-s2_x * s1_y + s1_x * s2_y);
  t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) /
      (-s2_x * s1_y + s1_x * s2_y);

  if (intersection) {
    intersection->x = p0_x + (t * s1_x);
    intersection->y = p0_y + (t * s1_y);
  }
  if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
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

const float epsilon = 1e-5;
std::vector<double>
Geometry::getFootprint(std::span<const glm::vec3> vertices) {
  std::vector<double> result;
  int idx = 0;
  for (auto &vertex : vertices) {
    float height = zUp ? vertex.z : vertex.y;
    if (abs(height - 0.f) < epsilon) {
      if (zUp) {
        result.push_back(vertex.x);
        result.push_back(vertex.y);
      } else {
        result.push_back(-vertex.x);
        result.push_back(vertex.z);
      }
    }
  }
  return result;
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
    cross = cross && lineIntersects2d(this->mPoints[0].x, this->mPoints[0].y,
                                      this->mPoints[3].x, this->mPoints[3].y,
                                      other.mPoints[0].x, other.mPoints[0].y,
                                      other.mPoints[3].x, other.mPoints[3].y,
                                      &result[0]);
    cross = cross && lineIntersects2d(this->mPoints[1].x, this->mPoints[1].y,
                                      this->mPoints[2].x, this->mPoints[2].y,
                                      other.mPoints[1].x, other.mPoints[1].y,
                                      other.mPoints[2].x, other.mPoints[2].y,
                                      &result[1]);

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

  int numSegments = line.size() - 1;

  auto lastSeg = LineSegment(line[0], line[1], width);

  geometry.mData.mVertices.push_back(fromGround(lastSeg.mPoints[0]));
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
  geometry.mData.mVertices.push_back(fromGround(lastSeg.mPoints[1]));
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);

  glm::vec2 uvDistance(0.0f, 0.0f);
  geometry.mData.mTexCoords.push_back(
      {0.0f, uvDistance[0], static_cast<float>(featureId)});
  geometry.mData.mTexCoords.push_back(
      {1.0f, uvDistance[1], static_cast<float>(featureId)});

  for (int i = 1; i < numSegments; i++) {
    auto nextSeg = LineSegment(line[i + 0], line[i + 1], width);

    auto crossPoints = lastSeg.crossPoints(nextSeg);

    geometry.mData.mVertices.push_back(fromGround(crossPoints[0]));
    throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
    geometry.mData.mVertices.push_back(fromGround(crossPoints[1]));
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

  geometry.mData.mVertices.push_back(fromGround(lastSeg.mPoints[3]));
  throw_if_nan(geometry.mData.mVertices[geometry.mData.mVertices.size() - 1]);
  geometry.mData.mVertices.push_back(fromGround(lastSeg.mPoints[2]));
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

  int vertIdx = 0;
  int faceIdx = 0;

  for (int i = 0; i < numSegments; i++) {
    auto &face = geometry.mData.mFaces[faceIdx++];

    face.resize(4);
    face[0] = (i * 2) + 0;
    face[1] = (i * 2) + 1;
    face[2] = (i * 2) + 3;
    face[3] = (i * 2) + 2;
  }

  return geometry;
}

Geometry Geometry::extrude2dMesh(const vector<glm::vec2> &in_vertices,
                                 float height, int featureId) {
  Geometry geometry;

  using Edge = std::pair<glm::vec2, glm::vec2>;
  using EdgeList = std::vector<Edge>;

  vector<glm::vec2> baseVertices;

  bool begin_eq_end = in_vertices[0] == in_vertices[in_vertices.size() - 1];

  baseVertices.insert(baseVertices.begin(), in_vertices.begin(),
                      in_vertices.end());

  if (begin_eq_end) {
    baseVertices.pop_back();
  }

  if (baseVertices.size() < 3) {
    throw std::runtime_error("Not enough vertices (<3), to create a mesh");
  }

  size_t numBaseVertices = baseVertices.size();

  EdgeList edges;

  float lastEdgeAngle = 0;
  float accumEdge = 0;

  glm::vec2 center(0);

  for (size_t i = 0; i < numBaseVertices; i++) {

    auto &v1 = baseVertices[i];

    bool lastV = i + 1 == numBaseVertices;
    auto &v2 = lastV ? baseVertices[0] : baseVertices[i + 1];

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
    //     newEdge.first.x, newEdge.first.y, newEdge.second.x, newEdge.second.y,
    //     &intersect)) {

    //     return nullptr;
    //   }
    // }

    edges.push_back(Edge(v1, v2));
  }

  if (accumEdge > 0.0) {

    for (size_t i = 0; i < numBaseVertices / 2; i++) {
      auto tmp = baseVertices[i];
      baseVertices[i] = baseVertices[numBaseVertices - i - 1];
      baseVertices[numBaseVertices - i - 1] = tmp;
    }
  }

  bool doExtrude = height != 0.f;

  geometry.mData.mVertices.resize(doExtrude ? numBaseVertices * 6
                                            : numBaseVertices);
  geometry.mData.mNormals.resize(geometry.mData.mVertices.size());
  geometry.mData.mTexCoords.resize(
      texCoordScale != 0.0f ? geometry.mData.mVertices.size() : 0);

  BBox bbox;

  for (size_t v = 0; v < numBaseVertices; v++) {
    const glm::vec2 &nv = baseVertices[v];

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

  for (size_t i = 0; i < numBaseVertices; i++) {
    geometry.mData.mFaces[0][i] = numBaseVertices - i - 1;
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

aiMesh *Geometry::Data::toMesh() const {
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

} // namespace GeoUtils