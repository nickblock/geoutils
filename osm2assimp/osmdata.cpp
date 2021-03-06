#include "osmdata.h"
#include "assimpconstruct.h"
#include "geomconvert.h"
#include <iostream>
#include <algorithm>

#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/area/geom_assembler.hpp>

#include "convertlatlng.h"

#include "common.h"

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

namespace GeoUtils
{

  OSMDataImport::OSMDataImport(AssimpConstruct &ac, const ViewFilterList &filters)
      : mCount(0),
        mAssimpConstruct(ac),
        mFilters(filters),
        mParentNode(nullptr)
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

  void OSMDataImport::way(const osmium::Way &way)
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

      process(feature);
    }
  }

  void OSMDataImport::node(const osmium::Node &node)
  {
  }

  void OSMDataImport::setParentAINode(aiNode *node)
  {
    mParentNode = node;

    mAssimpConstruct.addLocator(node);
  }

  void sanitizeName(string &name)
  {

    EngineBlock::replaceAll(name, "&", "&amp;");
  }

  void OSMDataImport::process(const OSMFeature &feature)
  {
    try
    {

      aiMesh *mesh = nullptr;

      //if it's just a locator
      if (feature.type() & OSMFeature::LOCATION)
      {
        glm::vec2 loc = feature.coords()[0];
        glm::vec3 glm_pos = GeomConvert::posFromLoc(loc.x, loc.y, 0.0f);

        aiNode *locatorNode = new aiNode;
        aiVector3t<float> asm_pos(glm_pos.x, glm_pos.y, glm_pos.z);
        aiMatrix4x4t<float>::Translation(asm_pos, locatorNode->mTransformation);

        locatorNode->mName.Set(feature.name());

        mAssimpConstruct.addLocator(locatorNode);

        return;
      }

      // if it's something that wants turning into a 3d mesh
      else if (feature.type() & (OSMFeature::BUILDING | OSMFeature::WATER) && feature.type() & OSMFeature::CLOSED)
      {
        mesh = GeomConvert::extrude2dMesh(feature.coords(), feature.height());
      }

      // if it's something that wants turning into a polygon spline
      else if (feature.type() & OSMFeature::HIGHWAY)
      {
        mesh = GeomConvert::polygonFromSpline(feature.coords(), 3.0);
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
          return;
        }

        mesh->mMaterialIndex = mAssimpConstruct.addMaterial(materialName, mMatColors[materialName]);

        string nameSanitize = feature.name();
        sanitizeName(nameSanitize);

        mAssimpConstruct.addMesh(mesh, nameSanitize, mParentNode);

        mCount++;

        return;
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

} // namespace GeoUtils