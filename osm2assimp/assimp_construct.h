#ifndef ASSIMP_CONSTRUCT_H
#define ASSIMP_CONSTRUCT_H

#include "common_geo.h"
#include "assimp/scene.h"
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <vector>
#include <map>
#include <string>

class AssimpConstruct 
{
public:
  AssimpConstruct();

  int write(const std::string& filename);
  void addMesh(aiMesh* mesh, std::string name = "", aiNode* parent = nullptr);
  void addLocator(aiNode* node);
  int addMaterial(std::string, glm::vec3 color);

  const std::string& formatsAvailableStr();
  bool checkFormat(std::string);


  static aiMesh* extrude2dMesh(const std::vector<glm::vec2>& baseVertices, float height);
  static aiMesh* polygonFromSpline(const std::vector<glm::vec2>& baseVertices, float width);

  void setConsolidateMesh(bool mega);
  void setZUp(bool zup);
  
protected:
  void buildMultipleMeshes(aiScene*);
  void buildMegaMesh(aiScene*);
  void addLocatorsToScene(aiScene*);

  static glm::vec3 upNormal();
  static glm::vec3 posFromLoc(double lon, double lat, double height);

  struct Material {
    std::string mName;
    glm::vec3 mColor;
  };

  Assimp::Exporter mExporter;

  std::string mFormatsAvailable;
  std::map<std::string, std::string> mExtMap;
  std::vector<aiMesh*> mMeshes;
  std::vector<std::string> mMeshNames;
  std::vector<aiNode*> mMeshParents;

  std::vector<aiNode*> mLocatorNodes;

  std::map<std::string, int> mMaterialNameMap;
  std::vector<Material> mMaterials;
  
  bool mMegaMesh;
  static bool mZUp;
};
#endif