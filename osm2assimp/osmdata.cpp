#include "osmdata.h"
#include "assimp_construct.h"
#include <iostream>
#include <algorithm>

#include <osmium/handler/node_locations_for_ways.hpp>

#include <osmium/area/assembler.hpp>
#include <osmium/area/multipolygon_collector.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/visitor.hpp>
#include <osmium/geom/coordinates.hpp>
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/geom/geos.hpp>
#include <osmium/area/geom_assembler.hpp>

#include "centerearthfixedconvert.h"

#include "common.h"

using std::string;
using std::cout;
using std::endl;
using std::vector;
using std::map;

OSMDataImport::OSMDataImport(AssimpConstruct& ac, const osmium::Box& extents, int filter )
: mCount(0), 
  mExtents(extents),
  mRefPoint(EngineBlock::CenterEarthFixedConvert::to_coords(EngineBlock::CenterEarthFixedConvert::refPoint)),
  mAssimpConstruct(ac),
  mFilter(filter),
  mParentNode(nullptr)
{
  std::locale::global(std::locale(""));

  mMatColors["ground"] = glm::vec3(1.0);
  mMatColors["road"] = glm::vec3(0.2);  
  mMatColors["water"] = glm::vec3(0.2, 0.4, 0.8);
  mMatColors["green"] = glm::vec3(0.2, 0.7, 0.1);
  mMatColors["red"] = glm::vec3(0.8, 0.1, 0.2);
  mMatColors["default"] = glm::vec3(0.7, 0.0, 0.5);
  mMatColors["building"] = glm::vec3(0.9, 0.1, 0.7);
}

bool wayInBox(const osmium::Way& way, const osmium::Box& box) {
  for(const auto& node : way.nodes()) {
    if(node.location().valid() && box.contains(node.location())) {
      return true;
    }
  }
  return false;
}

void OSMDataImport::way(const osmium::Way& way) 
{
  if(!mExtents.valid() || wayInBox(way, mExtents)) {

    OSMFeature feature(way, mRefPoint);

    if(feature.isValid()) {
      
      process(feature);
    }
  }
}

void OSMDataImport::node(const osmium::Node& node)
{
  if(node.tags().has_key("addr:housenumber")) {

    bool saveIt = false;

    for (const auto& tag : node.tags()) {

      string tagStr(tag.key());

      if(tagStr.size() > 4 && tagStr.substr(0, 5) == string("addr:")) {

        cout << "Got tag = " << tag.key() << " : " << tag.value() << endl;

        saveIt = true;
      }
    }

    if(saveIt) {
      OSMFeature(node, mRefPoint);
    }
  }
}

void OSMDataImport::setParentAINode(aiNode* node)
{
  mParentNode = node;

  mAssimpConstruct.addLocator(node);
}

void sanitizeName(string& name) {
  
  EngineBlock::replaceAll(name, "&", "&amp;");
}


void OSMDataImport::process(const OSMFeature& feature)
{
  try {

    aiMesh* mesh = nullptr;

    if(feature.type() & OSMFeature::LOCATION & mFilter ) 
    {
      aiNode* locatorNode = new aiNode;

      glm::vec2 glm_pos = feature.coords()[0];
      aiVector3t<float> asm_pos(glm_pos.x, glm_pos.y, 0.0f);
      aiMatrix4x4t<float>::Translation(asm_pos, locatorNode->mTransformation);

      locatorNode->mName.Set(feature.name());

      mAssimpConstruct.addLocator(locatorNode);
    }
    else if(feature.type() & (OSMFeature::BUILDING | OSMFeature::WATER) && feature.type() & OSMFeature::CLOSED) {
        
      mesh = AssimpConstruct::extrude2dMesh(feature.coords(), feature.height()); 
    }
    else if(feature.type() & OSMFeature::HIGHWAY & mFilter) {
      
      mesh = AssimpConstruct::polygonFromSpline(feature.coords(), 3.0);
    }
    if(mesh) {

      string materialName = "default";

      if(feature.type() == OSMFeature::BUILDING) {
        materialName = "building";
      }

      if(mesh->mNumFaces == 0 || mesh->mFaces[0].mNumIndices == 0) {
        return;
      }

      mesh->mMaterialIndex = mAssimpConstruct.addMaterial(materialName, mMatColors[materialName]); 

      string nameSanitize = feature.name();
      sanitizeName(nameSanitize);

      mAssimpConstruct.addMesh(mesh, nameSanitize, mParentNode);
      
      mCount++;
    }
    else {

      if(feature.type() & mFilter) {
        static bool failed = true;
        if(failed) {
          cout << "Failed to import some buildings, eg " << feature.name() << endl;
          failed = false;
        }
      }

    }
  
  } catch (std::out_of_range) {
    
    static bool once = true;
    if(once) {
      cout << "Failed to decode some nodes" << endl;
    }
  }
}