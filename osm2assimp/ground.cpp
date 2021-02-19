#include "ground.h"
#include "clipper.hpp"
#include "assimp/mesh.h"
#include "geomconvert.h"

namespace GeoUtils
{
  using ClipperLib::cInt;
  using ClipperLib::Clipper;
  using ClipperLib::ctDifference;
  using ClipperLib::IntPoint;
  using ClipperLib::pftEvenOdd;
  using ClipperLib::pftNonZero;
  using ClipperLib::ptClip;
  using ClipperLib::ptSubject;

  const int Precision = 10000000;

  IntPoint fromGLM(const glm::vec2 &vec2, int precision)
  {
    return IntPoint(static_cast<cInt>(vec2.x * precision), static_cast<cInt>(vec2.y * precision));
  }

  Path fromGLM(const std::vector<glm::vec2> &polygon, int precision)
  {
    Path path;
    for (const auto &p : polygon)
    {
      path << fromGLM(p, precision);
    }
    return path;
  }

  Ground::Ground(const std::vector<glm::vec2> &extents)
      : mExtents(fromGLM(extents, Precision))
  {
    for (auto &p : extents)
    {
      mBBox.add(glm::vec3(p, 0.0f));
    }
  }

  void Ground::addSubtraction(const std::vector<glm::vec2> &polygon)
  {
    mSubs.push_back(fromGLM(polygon, Precision));
  }

  aiMesh *Ground::getMesh()
  {
    Clipper clipper;

    clipper.AddPaths({mExtents}, ptSubject, true);

    clipper.AddPaths(mSubs, ptClip, true);

    Paths solution;

    clipper.Execute(ctDifference, solution);

    aiMesh *mesh = new aiMesh();

    mesh->mNumFaces = solution.size();
    mesh->mFaces = new aiFace[solution.size()];

    int vertexIdx = 0;
    for (size_t i = 0; i < solution.size(); i++)
    {
      auto &face = mesh->mFaces[i];
      auto &sol = solution[i];

      face.mNumIndices = sol.size();
      face.mIndices = new unsigned int[face.mNumIndices];

      for (size_t j = 0; j < sol.size(); j++)
      {
        face.mIndices[j] = vertexIdx++;
      }
    }

    mesh->mNumVertices = vertexIdx;
    mesh->mVertices = new aiVector3D[vertexIdx];
    mesh->mTextureCoords[0] = new aiVector3D[vertexIdx];
    mesh->mNormals = new aiVector3D[vertexIdx];
    mesh->mNumUVComponents[0] = 2;

    vertexIdx = 0;
    auto upNormal = GeomConvert::upNormal();
    for (size_t i = 0; i < solution.size(); i++)
    {
      auto &sol = solution[i];
      for (size_t j = 0; j < sol.size(); j++)
      {
        double X = static_cast<double>(sol[j].X) / Precision;
        double Y = static_cast<double>(sol[j].Y) / Precision;

        glm::vec3 vertex = GeomConvert::posFromLoc(X, Y, 0.f);
        glm::vec3 uv = mBBox.fraction({X, Y, 0.0});

        mesh->mVertices[vertexIdx] = {vertex.x, vertex.y, vertex.z};
        mesh->mNormals[vertexIdx] = {upNormal.x, upNormal.y, upNormal.z};
        mesh->mTextureCoords[0][vertexIdx] = {uv.x, uv.y, uv.z};

        vertexIdx++;
      }
    }

    return mesh;
  }
}