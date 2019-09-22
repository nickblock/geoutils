
#include "assimp_construct.h"

extern "C" {
#include "gfxpoly.h"
}

#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include "common.h"
#include <assert.h>
#include <iostream>

using std::string;
using std::vector;
using std::cout;
using std::endl;

bool AssimpConstruct::mZUp = false;

AssimpConstruct::AssimpConstruct()
: mMegaMesh(false)
{
  int numFormats = mExporter.GetExportFormatCount();

  for(int n=0; n<numFormats; n++) {
    const aiExportFormatDesc* desc = mExporter.GetExportFormatDescription(n);

    if(n != 0) mFormatsAvailable += string("|");

    mFormatsAvailable += desc->fileExtension;
    mExtMap[desc->fileExtension] = desc->id;
  }

}

void AssimpConstruct::setConsolidateMesh(bool mega)
{
  mMegaMesh = mega;
}

bool lineIntersects2d(float p0_x, float p0_y, float p1_x, float p1_y, float p2_x, float p2_y, float p3_x, float p3_y, glm::vec3* intersection)
{
  float s1_x, s1_y, s2_x, s2_y;
  s1_x = p1_x - p0_x;
  s1_y = p1_y - p0_y;
  s2_x = p3_x - p2_x;
  s2_y = p3_y - p2_y;

  float s, t;
  s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
  t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

  if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
  {
      if(intersection) {
        intersection->x = p0_x + (t * s1_x);
        intersection->z = p0_y + (t * s1_y);
        intersection->y = 0.f;
      }
      return true;
  }

  return false; // No collision
}

std::vector<glm::vec2> fromGFXPoly(gfxpoly_t* poly) {

  vector<glm::vec2> pointsEnd;

  if(poly && poly->strokes) {
    int strokeCount = 0;
    int pointCount = 0;
    gfxsegmentlist_t*strokes = poly->strokes;
    bool firstStroke = true;

    vector<vector<glm::vec2>> segments;
    glm::vec2 firstPoint;
    while(strokes) {

      assert(strokes->dir != DIR_UNKNOWN);

      // cout << "stroke dir " << (strokes->dir ? "DIR_DOWN" : "DIR_UP") << endl;
      vector<glm::vec2> plist;
      for(int i=0; i<strokes->num_points; i++) {
        
        int index = strokes->dir == DIR_UP ? i : strokes->num_points-1-i; 
        gridpoint_t& gp = strokes->points[index];
        glm::vec2 point(gp.x, gp.y);

        // cout << gp.x << " " << gp.y << endl;
        plist.push_back(point);

        if(firstStroke && i == 0) {
          firstPoint = point;
        }
      }
      // cout << endl;
      segments.push_back(plist);

      if(firstStroke) {
        firstStroke = false;
      }
      strokeCount++;
      pointCount += strokes->num_points;
      strokes = strokes->next;
    }

    int segmentsJoined = 0;
    vector<glm::vec2>* plist = nullptr;

    while(segmentsJoined < strokeCount) {
      for(auto& s : segments) {
        if(s[0] == firstPoint) {
          plist = &s;
          break;
        }
      }
      assert(plist);
      int beginAdd = 1;
      if(segmentsJoined == 0) beginAdd = 0;
      pointsEnd.insert(pointsEnd.end(), plist->begin()+beginAdd, plist->end());
      segmentsJoined++;
      firstPoint = (*plist)[plist->size()-1];
      plist = nullptr;
    }
  }
  return pointsEnd;
}
void AssimpConstruct::setZUp(bool z)
{
  mZUp = z;
}
glm::vec3 AssimpConstruct::upNormal()
{
  if(mZUp) {
    return glm::vec3(0.f, 0.f, 1.f);
  }
  else {
    return glm::vec3(0.f, 1.f, 0.f);
  }
}
glm::vec3 AssimpConstruct::posFromLoc(double lon, double lat, double height)
{
  if(mZUp) {
    return glm::vec3(lon, lat, height);
  }
  else {
    return glm::vec3(-lon, height, lat);
  }
}

aiMesh* AssimpConstruct::polygonFromSpline(const std::vector<glm::vec2>& vertices, float width)
{
  gfxline_t* outline = NULL;
  outline = gfxline_moveTo(outline, vertices[0].x, vertices[0].y);
  for(size_t i=1; i<vertices.size(); i++) {
    outline = gfxline_lineTo(outline, vertices[i].x, vertices[i].y);
  }
  gfxpoly_t* poly = gfxpoly_from_stroke(outline, width, gfx_capSquare, gfx_joinBevel, 0.0, 1.0);

  if(poly) {

    vector<glm::vec2> polyPoints = fromGFXPoly(poly);
    vector<glm::vec3> points3(polyPoints.size());
    for(size_t i=0; i<polyPoints.size(); i++) {
      points3[i] = posFromLoc(polyPoints[i].x, polyPoints[i].y, 0.5);
    }

    aiMesh* newMesh = new aiMesh;

    newMesh->mNumVertices = points3.size();
    newMesh->mVertices = new aiVector3D[points3.size()];
    newMesh->mNormals = new aiVector3D[points3.size()];

    glm::vec3 gUpNormal = AssimpConstruct::upNormal();
    aiVector3D aiUpNormal(gUpNormal.x, gUpNormal.y, gUpNormal.z);

    for(size_t i=0; i<newMesh->mNumVertices; i++) {
      newMesh->mNormals[i] = aiUpNormal;
    }

    memcpy(newMesh->mVertices, points3.data(), points3.size() * sizeof(glm::vec3));

    newMesh->mNumFaces = 1;
    newMesh->mFaces = new aiFace[1];
    aiFace& face = newMesh->mFaces[0];
    face.mNumIndices = points3.size();
    face.mIndices = new unsigned int[points3.size()];
    for(int i=0; i<points3.size(); i++) {
      face.mIndices[i] = i;
    }

    return newMesh;
  }
  
  return nullptr;
}

typedef std::pair<glm::vec2, glm::vec2> Edge;
typedef std::vector<Edge> EdgeList; 

aiMesh* AssimpConstruct::extrude2dMesh(const vector<glm::vec2>& in_vertices, float height)
{
  vector<glm::vec2> baseVertices;

  bool begin_eq_end = in_vertices[0] == in_vertices[in_vertices.size()-1];

  baseVertices.insert(baseVertices.begin(), in_vertices.begin(), in_vertices.end());

  if(begin_eq_end) {
    baseVertices.pop_back();
  }
  
  assert(baseVertices.size() > 2);

  size_t numBaseVertices = baseVertices.size();

  EdgeList edges;

  float lastEdgeAngle = 0;
  float accumEdge = 0;

  glm::vec2 center(0);
  for(size_t i=0; i<numBaseVertices; i++) {

    auto& v1 = baseVertices[i];

    bool lastV = i+1 == numBaseVertices;
    auto& v2 = lastV ? baseVertices[0] : baseVertices[i+1];

    center += v1;

    Edge newEdge(v1,v2);

    float edgeAngle = atan2(v2.x-v1.x, v2.y-v1.y);

    if(i != 0) {
      float edgeDiff = edgeAngle - lastEdgeAngle;
      if(edgeDiff > glm::pi<double>()) {
        edgeDiff -= glm::pi<double>();
      }
      if(edgeDiff < -glm::pi<double>()) {
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

    edges.push_back(Edge(v1,v2));
  }
  
  if(accumEdge > 0.0) 
  {

    for(size_t i=0; i<numBaseVertices/2; i++) {
      auto tmp = baseVertices[i];
      baseVertices[i] = baseVertices[numBaseVertices-i-1];
      baseVertices[numBaseVertices-i-1] = tmp;
    }
  }

  bool doExtrude = height != 0.f;

  vector<glm::vec3> vertices(doExtrude ? numBaseVertices*6 : numBaseVertices);
  vector<glm::vec3> normals(vertices.size());

  for(size_t v=0; v<numBaseVertices; v++) {
    const glm::vec2& nv = baseVertices[v];

    vertices[v] = posFromLoc(nv.x, nv.y, 0.0);
    normals[v] = -upNormal();
    if(height > 0.f) {
      vertices[v+numBaseVertices] = posFromLoc(nv.x, nv.y, height);
      normals[v+numBaseVertices] = upNormal();
    }
  }

  aiMesh* newMesh = new aiMesh();

  newMesh->mNumFaces = height > 0.f ? 2 + numBaseVertices : 1;
  newMesh->mFaces = new aiFace[newMesh->mNumFaces];

  newMesh->mFaces[0].mNumIndices = numBaseVertices;
  newMesh->mFaces[0].mIndices = new unsigned int[numBaseVertices];

  for(size_t i=0; i<numBaseVertices; i++) {
    newMesh->mFaces[0].mIndices[i] = numBaseVertices-i-1;
  }

  if(doExtrude) {
    
    newMesh->mFaces[1].mNumIndices = numBaseVertices;
    newMesh->mFaces[1].mIndices = new unsigned int[numBaseVertices];

    for(size_t i=0; i<numBaseVertices; i++) {
      newMesh->mFaces[1].mIndices[i] = numBaseVertices + i;
    }

    for(int f=0; f<numBaseVertices; f++) {

      int fn = f;
      if(f+1 == numBaseVertices) fn = -1;

      int index = numBaseVertices*2 + 4*f;
      glm::vec3* corners = &vertices[index];

      corners[3] = vertices[fn+1];
      corners[2] = vertices[f+0];
      corners[1] = vertices[f+numBaseVertices+0];
      corners[0] = vertices[fn+numBaseVertices+1];
      glm::vec3 v1 = corners[1] - corners[0];
      glm::vec3 v2 = corners[2] - corners[0];
      glm::vec3 n = glm::normalize(glm::cross(v1, v2));

      if(!mZUp) {
        n = -n;
      }

      ASSERTM( (glm::isnan(n.x) || glm::isnan(n.y) || glm::isnan(n.z)) == false , "Normal calc failed!");

      glm::vec3* vNormals = &normals[index];
      vNormals[0] = n;
      vNormals[1] = n;
      vNormals[2] = n;
      vNormals[3] = n;

      aiFace& face = newMesh->mFaces[2+f];
      face.mNumIndices = 4;
      face.mIndices = new unsigned int[4];   
      face.mIndices[0] = index+0;
      face.mIndices[1] = index+1;
      face.mIndices[2] = index+2;
      face.mIndices[3] = index+3;
    }
  }

  newMesh->mNumVertices = vertices.size();
  newMesh->mVertices = new aiVector3D[vertices.size()];
  newMesh->mNormals = new aiVector3D[vertices.size()];

  memcpy(newMesh->mVertices, vertices.data(), vertices.size() * sizeof(glm::vec3));
  memcpy(newMesh->mNormals, normals.data(), normals.size() * sizeof(glm::vec3));

  return newMesh;
}
void AssimpConstruct::buildMegaMesh(aiScene* assimpScene)
{
  assimpScene->mNumMeshes = 1;
  assimpScene->mMeshes = new aiMesh*[1];

  assimpScene->mRootNode = new aiNode;
  assimpScene->mRootNode->mNumMeshes = 0;
  assimpScene->mRootNode->mNumChildren = 1;
  assimpScene->mRootNode->mChildren = new aiNode*[1 + mLocatorNodes.size()];

  aiNode* node = new aiNode;
  assimpScene->mRootNode->mChildren[0] = node;
  node->mParent = assimpScene->mRootNode;
  node->mNumMeshes = 1;
  node->mMeshes = new unsigned int[1];
  node->mMeshes[0] = 0;

  const char* megaMeshName = "MegaMesh";
  node->mName.Set(megaMeshName);

  aiMesh* megaMesh = new aiMesh;
  assimpScene->mMeshes[0] = megaMesh;

  megaMesh->mNumVertices = 0;
  megaMesh->mNumFaces = 0;

  for(size_t i=0; i<mMeshes.size(); i++) {
    megaMesh->mNumVertices += mMeshes[i]->mNumVertices;
    megaMesh->mNumFaces += mMeshes[i]->mNumFaces; 
  }

  megaMesh->mVertices  = new aiVector3D[megaMesh->mNumVertices];
  megaMesh->mNormals   = new aiVector3D[megaMesh->mNumVertices];
  megaMesh->mFaces     = new aiFace[megaMesh->mNumFaces];


  int curVertices = 0;
  int curFaces = 0;

  for(size_t m=0; m<mMeshes.size(); m++) {

    memcpy( &megaMesh->mVertices[curVertices], 
            mMeshes[m]->mVertices, 
            mMeshes[m]->mNumVertices * sizeof(aiVector3D));

    memcpy( &megaMesh->mNormals[curVertices], 
            mMeshes[m]->mNormals, 
            mMeshes[m]->mNumVertices * sizeof(aiVector3D));

    for(size_t f=0; f<mMeshes[m]->mNumFaces; f++) {
      aiFace* srcFace = &mMeshes[m]->mFaces[f];
      aiFace* destFace = &megaMesh->mFaces[f + curFaces];

      destFace->mNumIndices = srcFace->mNumIndices;
      destFace->mIndices = new unsigned int[destFace->mNumIndices];

      for(size_t i=0; i<destFace->mNumIndices; i++) {
        destFace->mIndices[i] = srcFace->mIndices[i] + curVertices;
      }
    }

    curVertices += mMeshes[m]->mNumVertices;
    curFaces += mMeshes[m]->mNumFaces;
  }
}

void AssimpConstruct::buildMultipleMeshes(aiScene* assimpScene)
{
  assimpScene->mNumMeshes = mMeshes.size();
  assimpScene->mMeshes = mMeshes.data();

  assimpScene->mRootNode = new aiNode;
  assimpScene->mRootNode->mNumMeshes = 0;
  assimpScene->mRootNode->mNumChildren = mMeshes.size() + mLocatorNodes.size();
  assimpScene->mRootNode->mChildren = new aiNode*[mMeshes.size() + mLocatorNodes.size()];

  for(size_t i=0; i<mMeshes.size(); i++) {

    assimpScene->mRootNode->mChildren[i] = new aiNode();
    assimpScene->mRootNode->mChildren[i]->mParent = assimpScene->mRootNode;
    if(mMeshParents[i] != nullptr) {
      assimpScene->mRootNode->mChildren[i]->mParent = mMeshParents[i];
    }
    assimpScene->mRootNode->mChildren[i]->mNumMeshes = 1;
    assimpScene->mRootNode->mChildren[i]->mMeshes = new unsigned int[1];
    assimpScene->mRootNode->mChildren[i]->mMeshes[0] = i;
    assimpScene->mRootNode->mChildren[i]->mName.Set(mMeshNames[i]);
  }
}

void AssimpConstruct::addLocatorsToScene(aiScene* assimpScene)
{
  int index = mMeshes.size();
  for(auto& locNode : mLocatorNodes) {

    locNode->mParent = assimpScene->mRootNode;

    assimpScene->mRootNode->mChildren[index++] = locNode;
  }
}

int AssimpConstruct::write(const string& filename)
{

  aiScene* assimpScene = new aiScene;
  memset(assimpScene, 0, sizeof(aiScene));


  if(mMaterials.size() == 0) {

    assimpScene->mNumMaterials = 1;
    assimpScene->mMaterials = new aiMaterial*[1];
    assimpScene->mMaterials[0] = new aiMaterial();
    
  }
  else {
    assimpScene->mNumMaterials = mMaterials.size();
    assimpScene->mMaterials = new aiMaterial*[mMaterials.size()];
    size_t m=0;
    for(auto& matIter : mMaterials) {
      aiMaterial* material = new aiMaterial;
      aiString matName(matIter.mName);
      material->AddProperty(&matName, AI_MATKEY_NAME);
      glm::vec3& in_col = matIter.mColor;
      aiColor3D color(in_col.x, in_col.y, in_col.z);
      material->AddProperty(&color, 3, AI_MATKEY_COLOR_DIFFUSE);
      assimpScene->mMaterials[m++] = material;
    }
  }

  mMegaMesh ? buildMegaMesh(assimpScene) : buildMultipleMeshes(assimpScene);

  addLocatorsToScene(assimpScene);

  string outExt = filename.substr(filename.size()-3, 3);

  return mExporter.Export(assimpScene, mExtMap[outExt], filename, aiProcess_Triangulate);
}


void AssimpConstruct::addMesh(aiMesh* mesh, std::string name, aiNode* parent)
{
  mMeshes.push_back( mesh);
  mMeshNames.push_back(name);
  mMeshParents.push_back(parent);
}

void AssimpConstruct::addLocator(aiNode* node)
{
  mLocatorNodes.push_back(node);
}

int AssimpConstruct::addMaterial(std::string matname, glm::vec3 color) {
  const auto& iter = mMaterialNameMap.find(matname);
  if(iter != mMaterialNameMap.end()) {
    return iter->second;
  }
  else {
    int matIndex = mMaterialNameMap.size();
    mMaterialNameMap[matname] = matIndex;
    Material mat;
    mat.mName = matname;
    mat.mColor = color;
    mMaterials.push_back(mat);
    return matIndex;
  }
}

const std::string& AssimpConstruct::formatsAvailableStr()
{
  return mFormatsAvailable;
}
bool AssimpConstruct::checkFormat(std::string ext)
{
  return mExtMap.find(ext) != mExtMap.end();
}
