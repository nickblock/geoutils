#include "sceneconstruct.h"
#include "assimpwriter.h"
#include "geomconvert.h"
#include "common.h"
#include <iostream>

using std::cout;
using std::endl;
using std::string;

namespace GeoUtils
{
  void sanitizeName(string &name)
  {
    EngineBlock::replaceAll(name, "&", "&amp;");
  }
  SceneConstruct::SceneConstruct(const ViewFilterList &filters) : mFilters(filters)
  {
    std::locale::global(std::locale(""));

    mMatColors["ground"] = glm::vec3(149 / 255.f, 174 / 255.f, 81 / 255.f);
    mMatColors["highway"] = glm::vec3(81 / 255.f, 149 / 255.f, 174 / 255.f);
    mMatColors["water"] = glm::vec3(0.2, 0.4, 0.8);
    mMatColors["green"] = glm::vec3(0.2, 0.7, 0.1);
    mMatColors["red"] = glm::vec3(0.8, 0.1, 0.2);
    mMatColors["default"] = glm::vec3(174 / 255.f, 81 / 255.f, 149 / 255.f);
    mMatColors["building"] = glm::vec3(0.9, 0.1, 0.7);
  }

  void SceneConstruct::way(const osmium::Way &way)
  {
    for (const auto &filter : mFilters)
    {
      if (!filter->include(way))
      {
        return;
      }
    }

    OSMFeature feature(way);

    if (feature.isValid())
    {
      mFeatures.push_back(feature);
    }
  }

  void SceneConstruct::node(const osmium::Node &node)
  {
  }
  void SceneConstruct::addGround(const std::vector<glm::vec2> &groundCorners)
  {
    // float groundDepth = 5.0;
    // aiMesh *mesh = GeomConvert::extrude2dMesh(groundCorners, groundDepth, 0);
    // aiNode *parent = new aiNode;

    // aiMatrix4x4::Translation(zup ? aiVector3D(0.0, 0.0, -(groundDepth + 0.1)) : aiVector3D(0.0, -(groundDepth + 0.1), 0.0), parent->mTransformation);
    // mesh->mMaterialIndex = sceneConstruct.addMaterial("ground", glm::vec3(149 / 255.f, 174 / 255.f, 81 / 255.f));
    // sceneConstruct.addMesh(mesh, "ground", parent);
  }
  int SceneConstruct::write(const std::string &outFilePath, AssimpWriter &writer, const OutputConfig &config)
  {
    GeomConvert::zUp = config.mZUp;
    GeomConvert::texCoordScale = config.mTexCoordScale;

    int count;
    for (auto &feature : mFeatures)
    {
      try
      {
        aiMesh *mesh = nullptr;

        // if it's something that wants turning into a 3d mesh
        if (feature.type() & (OSMFeature::BUILDING | OSMFeature::WATER) && feature.type() & OSMFeature::CLOSED)
        {
          mesh = GeomConvert::extrude2dMesh(feature.coords(), feature.height(), count);
        }

        // if it's something that wants turning into a polygon spline
        else if (feature.type() & OSMFeature::HIGHWAY)
        {
          mesh = GeomConvert::meshFromLine(feature.coords(), OSMFeature::RoadWidth, count);
        }

        // if we made either kind of mesh successfully
        if (mesh)
        {
          string materialName = "default";

          if (feature.type() & OSMFeature::BUILDING)
          {
            materialName = "building";
          }
          else if (feature.type() & OSMFeature::HIGHWAY)
          {
            materialName = "highway";
          }
          else if (feature.type() & OSMFeature::WATER)
          {
            materialName = "water";
          }

          if (mesh->mNumFaces == 0 || mesh->mFaces[0].mNumIndices == 0)
          {
            continue;
          }

          mesh->mMaterialIndex = writer.addMaterial(materialName, mMatColors[materialName]);

          string nameSanitize = feature.name();
          sanitizeName(nameSanitize);

          writer.addMesh(mesh, nameSanitize, nullptr);

          count++;
        }
        else
        {

          static bool failed = true;
          if (failed)
          {
            cout << "Failed to import some buildings, eg " << feature.name() << endl;
            failed = false;
          }
        }
      }
      catch (std::out_of_range)
      {

        static bool once = true;
        if (once)
        {
          cout << "Failed to decode some nodes" << endl;
        }
      }
    }

    return writer.write(outFilePath);
  }
}