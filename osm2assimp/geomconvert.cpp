#include "geomconvert.h"
#include "common.h"
#include "glm/gtc/constants.hpp"
#include "utils.h"
#include <array>

using std::vector;

namespace GeoUtils {

bool GeomConvert::zUp = false;
float GeomConvert::texCoordScale = 0.0f;

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

glm::vec3 GeomConvert::upNormal() {
  if (zUp) {
    return glm::vec3(0.f, 0.f, 1.f);
  } else {
    return glm::vec3(0.f, 1.f, 0.f);
  }
}
glm::vec3 GeomConvert::posFromLoc(double lon, double lat, double height) {
  if (zUp) {
    return glm::vec3(lon, lat, height);
  } else {
    return glm::vec3(-lon, height, lat);
  }
}

glm::vec3 GeomConvert::fromGround(const glm::vec2 &groundCoords) {
  if (zUp) {
    return glm::vec3(groundCoords, 0.0f);
  } else {
    return {-groundCoords.x, 0.0f, groundCoords.y};
  }
}

const float epsilon = 1e-5;
std::vector<double>
GeomConvert::getFootprint(std::span<const glm::vec3> vertices) {
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

aiMesh *GeomConvert::meshFromLine(const std::vector<glm::vec2> &line,
                                  float width, int featureId) {
  if (line.size() < 2) {
    return nullptr;
  }
  int numSegments = line.size() - 1;

  std::vector<glm::vec3> points;
  std::vector<glm::vec3> texcoords;

  auto lastSeg = LineSegment(line[0], line[1], width);

  points.push_back(fromGround(lastSeg.mPoints[0]));
  throw_if_nan(points[points.size() - 1]);
  points.push_back(fromGround(lastSeg.mPoints[1]));
  throw_if_nan(points[points.size() - 1]);

  glm::vec2 uvDistance(0.0f, 0.0f);
  texcoords.push_back({0.0f, uvDistance[0], static_cast<float>(featureId)});
  texcoords.push_back({1.0f, uvDistance[1], static_cast<float>(featureId)});

  for (int i = 1; i < numSegments; i++) {
    auto nextSeg = LineSegment(line[i + 0], line[i + 1], width);

    auto crossPoints = lastSeg.crossPoints(nextSeg);

    points.push_back(fromGround(crossPoints[0]));
    throw_if_nan(points[points.size() - 1]);
    points.push_back(fromGround(crossPoints[1]));
    throw_if_nan(points[points.size() - 1]);

    uvDistance += glm::vec2{
        glm::distance(points[i * 2 + 0], points[i * 2 - 2]) / width,
        glm::distance(points[i * 2 + 1], points[i * 2 - 1]) / width,
    };

    texcoords.push_back({0.0f, uvDistance[0], static_cast<float>(featureId)});
    texcoords.push_back({1.0f, uvDistance[1], static_cast<float>(featureId)});

    lastSeg = nextSeg;
  }

  points.push_back(fromGround(lastSeg.mPoints[3]));
  throw_if_nan(points[points.size() - 1]);
  points.push_back(fromGround(lastSeg.mPoints[2]));
  throw_if_nan(points[points.size() - 1]);

  uvDistance += glm::vec2{
      glm::distance(points[points.size() - 1], points[points.size() - 3]) /
          width,
      glm::distance(points[points.size() - 2], points[points.size() - 4]) /
          width,
  };

  texcoords.push_back({0.0f, uvDistance[0], static_cast<float>(featureId)});
  texcoords.push_back({1.0f, uvDistance[1], static_cast<float>(featureId)});

  aiMesh *mesh = new aiMesh;

  mesh->mNumVertices = numSegments * 2 + 4;
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  mesh->mNumUVComponents[0] = 2;
  mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  memcpy(mesh->mTextureCoords[0], texcoords.data(),
         mesh->mNumVertices * sizeof(glm::vec3));

  mesh->mNumFaces = numSegments;
  mesh->mFaces = new aiFace[numSegments];

  memcpy(mesh->mVertices, points.data(), sizeof(glm::vec3) * points.size());

  static auto aiUpNormal = aiVector3D(upNormal().x, upNormal().y, upNormal().z);
  for (int i = 0; i < points.size(); i++) {
    mesh->mNormals[i] = aiUpNormal;
  }

  int vertIdx = 0;
  int faceIdx = 0;

  for (int i = 0; i < numSegments; i++) {
    auto &face = mesh->mFaces[faceIdx++];

    face.mNumIndices = 4;
    face.mIndices = new unsigned int[4];

    face.mIndices[0] = (i * 2) + 0;
    face.mIndices[1] = (i * 2) + 1;
    face.mIndices[2] = (i * 2) + 3;
    face.mIndices[3] = (i * 2) + 2;
  }

  return mesh;
}

aiMesh *GeomConvert::extrude2dMesh(const vector<glm::vec2> &in_vertices,
                                   float height, int featureId) {
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
    return nullptr;
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

  vector<glm::vec3> vertices(doExtrude ? numBaseVertices * 6 : numBaseVertices);
  vector<glm::vec3> normals(vertices.size());
  vector<glm::vec3> texcoords(texCoordScale != 0.0f ? vertices.size() : 0);

  BBox bbox;

  for (size_t v = 0; v < numBaseVertices; v++) {
    const glm::vec2 &nv = baseVertices[v];

    vertices[v] = posFromLoc(nv.x, nv.y, 0.0);
    normals[v] = -upNormal();

    bbox.add(vertices[v]);

    if (height > 0.f) {
      vertices[v + numBaseVertices] = posFromLoc(nv.x, nv.y, height);
      bbox.add(vertices[v + numBaseVertices]);
      normals[v + numBaseVertices] = upNormal();
    }
  }

  aiMesh *newMesh = new aiMesh();

  newMesh->mNumFaces = height > 0.f ? 2 + numBaseVertices : 1;
  newMesh->mFaces = new aiFace[newMesh->mNumFaces];

  newMesh->mFaces[0].mNumIndices = numBaseVertices;
  newMesh->mFaces[0].mIndices = new unsigned int[numBaseVertices];

  for (size_t i = 0; i < numBaseVertices; i++) {
    newMesh->mFaces[0].mIndices[i] = numBaseVertices - i - 1;
  }

  if (doExtrude) {

    newMesh->mFaces[1].mNumIndices = numBaseVertices;
    newMesh->mFaces[1].mIndices = new unsigned int[numBaseVertices];

    for (size_t i = 0; i < numBaseVertices; i++) {
      newMesh->mFaces[1].mIndices[i] = numBaseVertices + i;
    }

    for (int f = 0; f < numBaseVertices; f++) {

      int fn = f;
      if (f + 1 == numBaseVertices)
        fn = -1;

      int index = numBaseVertices * 2 + 4 * f;
      glm::vec3 *corners = &vertices[index];

      corners[3] = vertices[fn + 1];
      corners[2] = vertices[f + 0];
      corners[1] = vertices[f + numBaseVertices + 0];
      corners[0] = vertices[fn + numBaseVertices + 1];
      glm::vec3 v1 = corners[1] - corners[0];
      glm::vec3 v2 = corners[2] - corners[0];
      glm::vec3 n = glm::normalize(glm::cross(v1, v2));

      if (!zUp) {
        n = -n;
      }

      if (isnan(n)) {
        throw std::runtime_error("Normal calc failed!");
      }

      glm::vec3 *vNormals = &normals[index];
      vNormals[0] = n;
      vNormals[1] = n;
      vNormals[2] = n;
      vNormals[3] = n;

      if (texcoords.size()) {
        glm::vec3 *texCoord = &texcoords[index];

        float width = glm::distance(corners[0], corners[1]);
        float texCoordU = std::round(width / texCoordScale);
        float texCoordV = std::round(height / texCoordScale);

        texCoord[0] = {texCoordU, texCoordV, static_cast<float>(featureId)};
        texCoord[1] = {0.f, texCoordV, static_cast<float>(featureId)};
        texCoord[2] = {0.f, 0.f, static_cast<float>(featureId)};
        texCoord[3] = {texCoordU, 0.f, static_cast<float>(featureId)};
      }

      aiFace &face = newMesh->mFaces[2 + f];
      face.mNumIndices = 4;
      face.mIndices = new unsigned int[4];
      face.mIndices[0] = index + 0;
      face.mIndices[1] = index + 1;
      face.mIndices[2] = index + 2;
      face.mIndices[3] = index + 3;
    }
  }

  newMesh->mNumVertices = vertices.size();
  newMesh->mVertices = new aiVector3D[vertices.size()];
  newMesh->mNormals = new aiVector3D[vertices.size()];

  memcpy(newMesh->mVertices, vertices.data(),
         vertices.size() * sizeof(glm::vec3));
  memcpy(newMesh->mNormals, normals.data(), normals.size() * sizeof(glm::vec3));

  if (texcoords.size()) {
    newMesh->mNumUVComponents[0] = 2;
    newMesh->mTextureCoords[0] = new aiVector3D[vertices.size()];
    memcpy(newMesh->mTextureCoords[0], texcoords.data(),
           vertices.size() * sizeof(glm::vec3));
  }

  return newMesh;
}

} // namespace GeoUtils