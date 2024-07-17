#include "ground.h"
#include "assimp/mesh.h"
#include "geomconvert.h"
#include "poly2tri/poly2tri.h"
#include "tinyformat.h"
#include <fstream>
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::ofstream;
using std::stringstream;
using namespace tinyformat;

namespace GeoUtils {
Ground::Ground(const std::vector<glm::vec2> &extents) : mExtents(extents) {
  for (auto &p : extents) {
    mBBox.add(glm::vec3(p, 0.0f));
  }
}

void Ground::addSubtraction(const OSMFeature &feature) {
  mAdded++;

  bool merged = true;

  BoxPoly boxPoly = {feature.getBBox(), feature.coords()};
  while (merged) {
    merged = false;

    for (auto bIter = mSubs.begin(); bIter != mSubs.end(); ++bIter) {
      if (std::get<Box>(*bIter).overlaps(std::get<Box>(boxPoly))) {
        auto result =
            intersectPolygons(std::get<Poly>(boxPoly), std::get<Poly>(*bIter));

        if (result.size() == 1) {
          auto clean = cleanPolyon(result[0]);
          boxPoly = {bBoxFromPoints2D(clean), clean};
          merged = true;
          mSubs.erase(bIter);
          break;
        }
      }
    }
  }
  mSubs.push_back(boxPoly);

  mBBox.add(std::get<Box>(boxPoly));
}

std::vector<p2t::Point *> fromGLM(const std::vector<glm::vec2> points) {
  std::vector<p2t::Point *> result(points.size());

  for (int i = 0; i < points.size(); i++) {
    result[i] = new p2t::Point(points[i].x, points[i].y);
  }
  return result;
}

void freePointList(std::vector<p2t::Point *> points) {
  for (auto p : points) {
    delete p;
  }
}

stringstream pointsss(const std::vector<glm::vec2> &points,
                      const glm::vec2 &min, float scale) {
  stringstream ss;

  ss << "<polygon points=\"";

  for (auto &p : points) {
    ss << tfm::format("%d, %d, ", (p.x - min.x) * scale, (p.y - min.y) * scale);
  }

  ss << "\" fill=\"none\" stroke=\"black\" />";

  return ss;
}

void Ground::writeSvg(const std::string &path, float scale) {

  ofstream file = ofstream(path);

  file << tfm::format("<svg viewBox = \"%d %d %d %d\" xmlns = "
                      "\"http://www.w3.org/2000/svg\">",
                      0, 0, (mBBox.mMax.x - mBBox.mMin.x) * scale,
                      (mBBox.mMax.y - mBBox.mMin.y) * scale)
       << endl;

  for (auto &iter : mSubs) {
    file << pointsss(std::get<Poly>(iter), mBBox.mMin, scale).str() << endl;
  }

  file << "</svg>" << endl;
}

aiMesh *Ground::getMesh() {

  cout << "Features added = " << mAdded << " polys = " << mSubs.size() << endl;

  writeSvg("/tmp/ground.svg", 10.f);

  float extra = 1.f;
  std::vector<glm::vec2> boxPoints = {
      {mBBox.mMin.x - extra, mBBox.mMin.y - extra},
      {mBBox.mMin.x - extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMax.y + extra},
      {mBBox.mMax.x + extra, mBBox.mMin.y - extra}};

  auto boxP2t = fromGLM(boxPoints);
  p2t::CDT cdt(boxP2t);

  std::vector<std::vector<p2t::Point *>> p2tPoints;
  for (auto &p : mSubs) {
    auto poly = std::get<Poly>(p);
    poly.pop_back();
    auto p2tP = fromGLM(poly);
    p2tPoints.push_back(p2tP);
    cdt.AddHole(p2tP);
  }

  cdt.Triangulate();

  auto tris = cdt.GetTriangles();

  aiMesh *mesh = new aiMesh();

  mesh->mNumVertices = tris.size() * 3;
  mesh->mVertices = new aiVector3D[mesh->mNumVertices];
  mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
  mesh->mNormals = new aiVector3D[mesh->mNumVertices];
  mesh->mNumUVComponents[0] = 2;

  mesh->mNumFaces = tris.size();
  mesh->mFaces = new aiFace[tris.size()];

  int vertexIdx = 0;
  auto upNormal = GeomConvert::upNormal();
  for (size_t i = 0; i < tris.size(); i++) {
    auto &face = mesh->mFaces[i];

    face.mNumIndices = 3;
    face.mIndices = new unsigned int[face.mNumIndices];

    for (size_t j = 0; j < 3; j++) {
      face.mIndices[j] = vertexIdx;

      auto point = tris[i]->GetPoint(j);

      glm::vec3 vertex = GeomConvert::posFromLoc(point->x, point->y, 0.f);
      glm::vec3 uv = mBBox.fraction({point->x, point->y, 0.0});

      mesh->mVertices[vertexIdx] = {vertex.x, vertex.y, vertex.z};
      mesh->mNormals[vertexIdx] = {upNormal.x, upNormal.y, upNormal.z};
      mesh->mTextureCoords[0][vertexIdx] = {uv.x, uv.y, uv.z};

      vertexIdx++;
    }
  }

  freePointList(boxP2t);

  for (auto &p : p2tPoints) {
    freePointList(p);
  }

  return mesh;
}
} // namespace GeoUtils