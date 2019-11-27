#pragma once

#include "assimp/scene.h"
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

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

  size_t numMeshes() {
    return mMeshes.size();
  }

  void setConsolidateMesh(bool mega);
  
protected:
  void buildMultipleMeshes(aiScene*);
  void buildMegaMesh(aiScene*);
  void addLocatorsToScene(aiScene*);

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