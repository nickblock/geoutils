#include "geomconvert.h"
#include "common.h"

#include "clipper.hpp"
#include "glm/gtc/constants.hpp"

using std::vector;

namespace GeoUtils
{

  bool GeomConvert::zUp = false;

  bool lineIntersects2d(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, glm::vec3 *intersection)
  {
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;
    s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;
    s2_y = p3_y - p2_y;

    float s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
      if (intersection)
      {
        intersection->x = p0_x + (t * s1_x);
        intersection->z = p0_y + (t * s1_y);
        intersection->y = 0.f;
      }
      return true;
    }

    return false; // No collision
  }

  glm::vec3 GeomConvert::upNormal()
  {
    if (zUp)
    {
      return glm::vec3(0.f, 0.f, 1.f);
    }
    else
    {
      return glm::vec3(0.f, 1.f, 0.f);
    }
  }
  glm::vec3 GeomConvert::posFromLoc(double lon, double lat, double height)
  {
    if (zUp)
    {
      return glm::vec3(lon, lat, height);
    }
    else
    {
      return glm::vec3(-lon, height, lat);
    }
  }

  aiMesh *GeomConvert::polygonFromSpline(const std::vector<glm::vec2> &vertices, float width)
  {
    ClipperLib::ClipperOffset clipperOffset;

    ClipperLib::Path inputPath;

    constexpr int FloatToIntMultiplier = 100000;

    for (auto &v : vertices)
    {
      inputPath.push_back({static_cast<int>(v.x * FloatToIntMultiplier),
                           static_cast<int>(v.y * FloatToIntMultiplier)});
    }

    clipperOffset.AddPath(inputPath, ClipperLib::jtMiter, ClipperLib::etOpenSquare);

    ClipperLib::Paths solution;

    clipperOffset.Execute(solution, width * FloatToIntMultiplier);

    if (solution.size() > 0)
    {

      int totalVertices = 0;
      for (auto &path : solution)
      {
        totalVertices += path.size();
      }
      aiMesh *mesh = new aiMesh;

      mesh->mNumVertices = solution[0].size();
      mesh->mVertices = new aiVector3D[mesh->mNumVertices];
      mesh->mNormals = new aiVector3D[mesh->mNumVertices];

      glm::vec3 gUpNormal = GeomConvert::upNormal();
      aiVector3D aiUpNormal(gUpNormal.x, gUpNormal.y, gUpNormal.z);

      for (size_t i = 0; i < mesh->mNumVertices; i++)
      {
        mesh->mNormals[i] = aiUpNormal;
      }

      mesh->mNumFaces = 1;
      mesh->mFaces = new aiFace[1];

      aiFace &aFace = mesh->mFaces[0];
      aFace.mNumIndices = mesh->mNumVertices;
      aFace.mIndices = new unsigned int[aFace.mNumIndices];

      int idx = 0;
      for (auto &p : solution[0])
      {
        glm::vec3 pv = posFromLoc(static_cast<double>(p.X) / FloatToIntMultiplier, static_cast<double>(p.Y) / FloatToIntMultiplier, 0.5);

        aiVector3D &av = mesh->mVertices[idx];
        av.x = pv.x;
        av.y = pv.y;
        av.z = pv.z;

        aFace.mIndices[idx] = idx;

        idx++;
      }

      return mesh;
    }
    else
    {
      return nullptr;
    }
  }

  aiMesh *GeomConvert::extrude2dMesh(const vector<glm::vec2> &in_vertices, float height)
  {
    using Edge = std::pair<glm::vec2, glm::vec2>;
    using EdgeList = std::vector<Edge>;

    vector<glm::vec2> baseVertices;

    bool begin_eq_end = in_vertices[0] == in_vertices[in_vertices.size() - 1];

    baseVertices.insert(baseVertices.begin(), in_vertices.begin(), in_vertices.end());

    if (begin_eq_end)
    {
      baseVertices.pop_back();
    }

    if (baseVertices.size() < 3)
    {
      return nullptr;
    }

    size_t numBaseVertices = baseVertices.size();

    EdgeList edges;

    float lastEdgeAngle = 0;
    float accumEdge = 0;

    glm::vec2 center(0);
    for (size_t i = 0; i < numBaseVertices; i++)
    {

      auto &v1 = baseVertices[i];

      bool lastV = i + 1 == numBaseVertices;
      auto &v2 = lastV ? baseVertices[0] : baseVertices[i + 1];

      center += v1;

      Edge newEdge(v1, v2);

      float edgeAngle = atan2(v2.x - v1.x, v2.y - v1.y);

      if (i != 0)
      {
        float edgeDiff = edgeAngle - lastEdgeAngle;
        if (edgeDiff > glm::pi<double>())
        {
          edgeDiff -= glm::pi<double>();
        }
        if (edgeDiff < -glm::pi<double>())
        {
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
      //   if(lineIntersects2d(other.first.x, other.first.y, other.second.x, other.second.y,
      //     newEdge.first.x, newEdge.first.y, newEdge.second.x, newEdge.second.y, &intersect)) {

      //     return nullptr;
      //   }
      // }

      edges.push_back(Edge(v1, v2));
    }

    if (accumEdge > 0.0)
    {

      for (size_t i = 0; i < numBaseVertices / 2; i++)
      {
        auto tmp = baseVertices[i];
        baseVertices[i] = baseVertices[numBaseVertices - i - 1];
        baseVertices[numBaseVertices - i - 1] = tmp;
      }
    }

    bool doExtrude = height != 0.f;

    vector<glm::vec3> vertices(doExtrude ? numBaseVertices * 6 : numBaseVertices);
    vector<glm::vec3> normals(vertices.size());

    for (size_t v = 0; v < numBaseVertices; v++)
    {
      const glm::vec2 &nv = baseVertices[v];

      vertices[v] = posFromLoc(nv.x, nv.y, 0.0);
      normals[v] = -upNormal();
      if (height > 0.f)
      {
        vertices[v + numBaseVertices] = posFromLoc(nv.x, nv.y, height);
        normals[v + numBaseVertices] = upNormal();
      }
    }

    aiMesh *newMesh = new aiMesh();

    newMesh->mNumFaces = height > 0.f ? 2 + numBaseVertices : 1;
    newMesh->mFaces = new aiFace[newMesh->mNumFaces];

    newMesh->mFaces[0].mNumIndices = numBaseVertices;
    newMesh->mFaces[0].mIndices = new unsigned int[numBaseVertices];

    for (size_t i = 0; i < numBaseVertices; i++)
    {
      newMesh->mFaces[0].mIndices[i] = numBaseVertices - i - 1;
    }

    if (doExtrude)
    {

      newMesh->mFaces[1].mNumIndices = numBaseVertices;
      newMesh->mFaces[1].mIndices = new unsigned int[numBaseVertices];

      for (size_t i = 0; i < numBaseVertices; i++)
      {
        newMesh->mFaces[1].mIndices[i] = numBaseVertices + i;
      }

      for (int f = 0; f < numBaseVertices; f++)
      {

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

        if (!zUp)
        {
          n = -n;
        }

        ASSERTM((glm::isnan(n.x) || glm::isnan(n.y) || glm::isnan(n.z)) == false, "Normal calc failed!");

        glm::vec3 *vNormals = &normals[index];
        vNormals[0] = n;
        vNormals[1] = n;
        vNormals[2] = n;
        vNormals[3] = n;

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

    memcpy(newMesh->mVertices, vertices.data(), vertices.size() * sizeof(glm::vec3));
    memcpy(newMesh->mNormals, normals.data(), normals.size() * sizeof(glm::vec3));

    return newMesh;
  }

} // namespace GeoUtils