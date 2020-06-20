#pragma once

#include "assimp/scene.h"
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#include <vector>
#include <map>
#include <string>

namespace GeoUtils
{

  /// <summary>
  /// A class represeting an assimp scene
  /// Meshes and Locators are added to the object and then the write function can be called
  /// to write whole sceene to file.
  /// <summary>
  class AssimpConstruct
  {
  public:
    enum MeshGranularity
    {
      OneBigMesh = 0,
      MeshPerMaterial = 1,
      MeshPerObject = 2
    };

    AssimpConstruct();

    int write(const std::string &filename);
    void addMesh(aiMesh *mesh, std::string name = "", aiNode *parent = nullptr);
    void addLocator(aiNode *node);
    int addMaterial(std::string, glm::vec3 color);

    const std::string &formatsAvailableStr();
    bool checkFormat(std::string);

    size_t numMeshes()
    {
      return mMeshes.size();
    }

    const char *exporterErrorStr()
    {
      return mExporter.GetErrorString();
    }

    /// <summary>
    /// set the level of granularity of mesh export
    /// </summary>
    void setMeshGranularity(MeshGranularity m);

  protected:
    /// <summary>
    /// Center the mesh vertices around their average point and translate the parent node.
    /// </summary>
    void centerMeshes(aiScene *);
    void buildMeshPerObject(aiScene *);
    void buildMeshPerMaterial(aiScene *);
    void buildMegaMesh(aiScene *);
    void addLocatorsToScene(aiScene *);

    using NodeAndMesh = std::tuple<aiMesh *, aiNode *>;

    // take a list of meshes and return a node with one merged mesh
    // pass in the mesh index counted from the number of mesh in the scene
    NodeAndMesh mergeMeshes(std::vector<aiMesh *> meshes, int meshIdx);

    struct Material
    {
      std::string mName;
      glm::vec3 mColor;
    };

    Assimp::Exporter mExporter;

    std::string mFormatsAvailable;
    std::map<std::string, std::string> mExtMap;
    std::vector<aiMesh *> mMeshes;
    std::vector<std::string> mMeshNames;
    std::vector<aiNode *> mMeshParents;

    std::vector<aiNode *> mLocatorNodes;

    std::map<std::string, int> mMaterialNameMap;
    std::vector<Material> mMaterials;

    MeshGranularity mGran = MeshPerMaterial;
    static bool mZUp;
  };

} // namespace GeoUtils