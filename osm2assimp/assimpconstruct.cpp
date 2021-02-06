
#include "assimpconstruct.h"
#include "osmdata.h"

#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include "common.h"
#include <assert.h>
#include <iostream>

using std::cout;
using std::endl;
using std::string;
using std::vector;

namespace GeoUtils
{
  bool AssimpConstruct::mZUp = false;

  AssimpConstruct::AssimpConstruct()
  {
    int numFormats = mExporter.GetExportFormatCount();

    for (int n = 0; n < numFormats; n++)
    {
      const aiExportFormatDesc *desc = mExporter.GetExportFormatDescription(n);

      if (n != 0)
        mFormatsAvailable += string("|");

      mFormatsAvailable += desc->fileExtension;
      mExtMap[desc->fileExtension] = desc->id;
    }
  }

  void AssimpConstruct::setMeshGranularity(MeshGranularity m)
  {
    mGran = m;
  }

  AssimpConstruct::NodeAndMesh AssimpConstruct::mergeMeshes(std::vector<aiMesh *> meshes, int meshIdx)
  {
    aiNode *node = new aiNode;
    node->mNumMeshes = 1;
    node->mMeshes = new unsigned int[1];
    node->mMeshes[0] = meshIdx;

    aiMesh *mergedMesh = new aiMesh;

    mergedMesh->mNumVertices = 0;
    mergedMesh->mNumFaces = 0;
    mergedMesh->mNumUVComponents[0] = meshes[0]->mNumUVComponents[0];

    for (size_t i = 0; i < meshes.size(); i++)
    {
      mergedMesh->mNumVertices += meshes[i]->mNumVertices;
      mergedMesh->mNumFaces += meshes[i]->mNumFaces;
    }

    mergedMesh->mVertices = new aiVector3D[mergedMesh->mNumVertices];
    mergedMesh->mNormals = new aiVector3D[mergedMesh->mNumVertices];
    mergedMesh->mFaces = new aiFace[mergedMesh->mNumFaces];

    if (mergedMesh->mNumUVComponents[0])
    {
      mergedMesh->mTextureCoords[0] = new aiVector3D[mergedMesh->mNumVertices];
    }

    int curVertices = 0;
    int curFaces = 0;

    for (size_t m = 0; m < meshes.size(); m++)
    {

      memcpy(&mergedMesh->mVertices[curVertices],
             meshes[m]->mVertices,
             meshes[m]->mNumVertices * sizeof(aiVector3D));

      memcpy(&mergedMesh->mNormals[curVertices],
             meshes[m]->mNormals,
             meshes[m]->mNumVertices * sizeof(aiVector3D));

      if (mergedMesh->mNumUVComponents[0])
      {
        memcpy(&mergedMesh->mTextureCoords[0][curVertices],
               meshes[m]->mTextureCoords[0],
               meshes[m]->mNumVertices * sizeof(aiVector3D));
      }

      for (size_t f = 0; f < meshes[m]->mNumFaces; f++)
      {
        aiFace *srcFace = &meshes[m]->mFaces[f];
        aiFace *destFace = &mergedMesh->mFaces[f + curFaces];

        destFace->mNumIndices = srcFace->mNumIndices;
        destFace->mIndices = new unsigned int[destFace->mNumIndices];

        for (size_t i = 0; i < destFace->mNumIndices; i++)
        {
          destFace->mIndices[i] = srcFace->mIndices[i] + curVertices;
        }
      }

      curVertices += meshes[m]->mNumVertices;
      curFaces += meshes[m]->mNumFaces;
    }

    return {mergedMesh, node};
  }

  void AssimpConstruct::buildMeshPerMaterial(aiScene *assimpScene)
  {
    std::map<int, std::vector<aiMesh *>> materialMeshMap;

    for (int i = 0; i < mMeshes.size(); i++)
    {
      if (materialMeshMap.find(mMeshes[i]->mMaterialIndex) == materialMeshMap.end())
      {
        materialMeshMap[mMeshes[i]->mMaterialIndex] = {};
      }
      freezeMesh(mMeshes[i], mMeshParents[i]);
      materialMeshMap[mMeshes[i]->mMaterialIndex]
          .push_back(mMeshes[i]);
    }

    assimpScene->mNumMeshes = materialMeshMap.size();
    assimpScene->mMeshes = new aiMesh *[materialMeshMap.size()];

    assimpScene->mRootNode = new aiNode;
    assimpScene->mRootNode->mNumMeshes = 0;
    assimpScene->mRootNode->mNumChildren = materialMeshMap.size();
    assimpScene->mRootNode->mChildren = new aiNode *[materialMeshMap.size()];

    int idx = 0;
    for (auto meshIter : materialMeshMap)
    {
      std::tie(
          assimpScene->mMeshes[idx],
          assimpScene->mRootNode->mChildren[idx]) = mergeMeshes(meshIter.second, idx);

      assimpScene->mMeshes[idx]->mMaterialIndex = meshIter.first;
      assimpScene->mMeshes[idx]->mName.Set(mMaterials[meshIter.first].mName.c_str());
      assimpScene->mRootNode->mChildren[idx]->mName.Set(mMaterials[meshIter.first].mName.c_str());
      idx++;
    }
  }

  void AssimpConstruct::buildMegaMesh(aiScene *assimpScene)
  {
    assimpScene->mNumMeshes = 1;
    assimpScene->mMeshes = new aiMesh *[1];

    assimpScene->mRootNode = new aiNode;
    assimpScene->mRootNode->mNumMeshes = 0;
    assimpScene->mRootNode->mNumChildren = 1;
    assimpScene->mRootNode->mChildren = new aiNode *[1];

    aiNode *node = new aiNode;
    assimpScene->mRootNode->mChildren[0] = node;
    node->mParent = assimpScene->mRootNode;
    node->mNumMeshes = 1;
    node->mMeshes = new unsigned int[1];
    node->mMeshes[0] = 0;

    const char *megaMeshName = "MegaMesh";
    node->mName.Set(megaMeshName);

    std::tie(
        assimpScene->mMeshes[0],
        assimpScene->mRootNode->mChildren[0]) = mergeMeshes(mMeshes, 0);
  }

  void AssimpConstruct::buildMeshPerObject(aiScene *assimpScene)
  {
    assimpScene->mNumMeshes = mMeshes.size();
    assimpScene->mMeshes = mMeshes.data();

    assimpScene->mRootNode = new aiNode;
    assimpScene->mRootNode->mNumMeshes = 0;
    assimpScene->mRootNode->mNumChildren = mMeshes.size() + mLocatorNodes.size();
    assimpScene->mRootNode->mChildren = new aiNode *[mMeshes.size() + mLocatorNodes.size()];

    for (size_t i = 0; i < mMeshes.size(); i++)
    {
      auto meshParent = mMeshParents[i];
      if (mMeshParents[i] != nullptr)
      {
        auto meshParent = mMeshParents[i];
        assimpScene->mRootNode->mChildren[i] = meshParent;
      }
      else
      {
        assimpScene->mRootNode->mChildren[i] = new aiNode();
      }
      assimpScene->mRootNode->mChildren[i]->mParent = assimpScene->mRootNode;
      assimpScene->mRootNode->mChildren[i]->mNumMeshes = 1;
      assimpScene->mRootNode->mChildren[i]->mMeshes = new unsigned int[1];
      assimpScene->mRootNode->mChildren[i]->mMeshes[0] = i;
      assimpScene->mRootNode->mChildren[i]->mName.Set(mMeshNames[i]);
    }

    centerMeshes(assimpScene);
  }

  void AssimpConstruct::addLocatorsToScene(aiScene *assimpScene)
  {
    int index = mMeshes.size();
    for (auto &locNode : mLocatorNodes)
    {

      locNode->mParent = assimpScene->mRootNode;

      assimpScene->mRootNode->mChildren[index++] = locNode;
    }
  }

  int AssimpConstruct::write(const string &filename)
  {

    aiScene *assimpScene = new aiScene;
    memset(assimpScene, 0, sizeof(aiScene));

    if (mMaterials.size() == 0)
    {

      assimpScene->mNumMaterials = 1;
      assimpScene->mMaterials = new aiMaterial *[1];
      assimpScene->mMaterials[0] = new aiMaterial();
    }
    else
    {
      assimpScene->mNumMaterials = mMaterials.size();
      assimpScene->mMaterials = new aiMaterial *[mMaterials.size()];
      size_t m = 0;
      for (auto &matIter : mMaterials)
      {
        aiMaterial *material = new aiMaterial;
        aiString matName(matIter.mName);
        material->AddProperty(&matName, AI_MATKEY_NAME);
        glm::vec3 &in_col = matIter.mColor;
        aiColor3D color(in_col.x, in_col.y, in_col.z);
        material->AddProperty(&color, 3, AI_MATKEY_COLOR_DIFFUSE);
        assimpScene->mMaterials[m++] = material;
      }
    }
    switch (mGran)
    {
    case OneBigMesh:
      buildMegaMesh(assimpScene);
      break;
    case MeshPerMaterial:
      buildMeshPerMaterial(assimpScene);
      break;
    case MeshPerObject:
      buildMeshPerObject(assimpScene);
      break;
    }

    string outExt = filename.substr(filename.find_last_of('.') + 1);

    return mExporter.Export(assimpScene, outExt, filename, aiProcess_Triangulate);
  }

  void AssimpConstruct::addMesh(aiMesh *mesh, std::string name, aiNode *parent)
  {
    mMeshes.push_back(mesh);
    mMeshNames.push_back(name);
    mMeshParents.push_back(parent);
  }

  void AssimpConstruct::addLocator(aiNode *node)
  {
    mLocatorNodes.push_back(node);
  }

  int AssimpConstruct::addMaterial(std::string matname, glm::vec3 color)
  {
    const auto &iter = mMaterialNameMap.find(matname);
    if (iter != mMaterialNameMap.end())
    {
      return iter->second;
    }
    else
    {
      int matIndex = mMaterialNameMap.size();
      mMaterialNameMap[matname] = matIndex;
      Material mat;
      mat.mName = matname;
      mat.mColor = color;
      mMaterials.push_back(mat);
      return matIndex;
    }
  }

  void AssimpConstruct::centerMeshes(aiScene *assimpScene)
  {
    for (int i = 0; i < assimpScene->mNumMeshes; i++)
    {

      auto mesh = assimpScene->mMeshes[i];
      auto node = assimpScene->mRootNode->mChildren[i];

      aiVector3D totalVec(0.0, 0.0, 0.0);

      for (int v = 0; v < mesh->mNumVertices; v++)
      {
        totalVec += mesh->mVertices[v];
      }

      totalVec /= mesh->mNumVertices;

      for (int v = 0; v < mesh->mNumVertices; v++)
      {
        mesh->mVertices[v] -= totalVec;
      }
      aiMatrix4x4 centerMat;
      aiMatrix4x4::Translation(totalVec, centerMat);
      node->mTransformation *= centerMat;
    }
  }

  void AssimpConstruct::freezeMesh(aiMesh *mesh, aiNode *parent)
  {
    if (parent == nullptr || parent->mTransformation.IsIdentity())
    {
      return;
    }

    for (int i = 0; i < mesh->mNumVertices; i++)
    {
      mesh->mVertices[i] *= parent->mTransformation;
    }
    for (int i = 0; i < mesh->mNumVertices; i++)
    {
      mesh->mNormals[i] *= parent->mTransformation;
      mesh->mNormals[i].Normalize();
    }
  }

  const std::string &AssimpConstruct::formatsAvailableStr()
  {
    return mFormatsAvailable;
  }
  bool AssimpConstruct::checkFormat(std::string ext)
  {
    return mExtMap.find(ext) != mExtMap.end();
  }

} // namespace GeoUtils