#include "osmfeature.h"
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/area/geom_assembler.hpp>
#include <osmium/geom/geos.hpp>

#include <sstream>

#include "centerearthfixedconvert.h"

using std::cout;
using std::endl;
using std::string;


std::vector<std::vector<std::string>> NameTags = {
  {"name"}, {"addr:housename"},
  {"addr:housenumber", "addr:street"}
};


std::string getName(const osmium::Way& way) 
{
  std::string name;
  for(const auto& taglist : NameTags) {
    bool foundAll = true;
    for(const auto& tag : taglist) {
      if(way.tags().has_key(tag.c_str())) {
        name += std::string(way.tags().get_value_by_key(tag.c_str(), "noname")) + std::string(" ");
      }
      else {
        foundAll = false;
        break;
      }
    }
    if(foundAll) break;

    else {
      name = std::string();
    }
  }
  if(name.size() == 0) {
      std::stringstream ss;
      ss << way.id();
      name = string(ss.str());
  }
  return name;
}

OSMFeature::OSMFeature(const osmium::Way& way, const WorldCoord& ref, bool findName)
:mType(UNDEFINED), mHeight(0.f), mFloors(3), mMaterial("default"), mValid(true)
{
  try {
    
    if(findName) {
      mName = getName(way);
    }
    else {
      mName = std::to_string(way.id());
    }

    bool setHeight = false;

    mHeight = buildingFloorHeight * mFloors;

    if(way.tags().has_key("height")) {

      try {
        mHeight = std::stof(way.tags()["height"]);
      }
      catch(std::invalid_argument) {

       mValid = false; 
      }
    }
    else if(way.tags().has_key("building:levels")) {
      
      try {
        mFloors = std::stoi(way.tags()["building:levels"]);
      }
      catch (std::invalid_argument) {
        mFloors = 1;
      }
      
      mHeight = mFloors * buildingFloorHeight;
      
    }

    if(way.tags().has_key("building")) {
      mType = BUILDING;
      mMaterial = "building";
    }
    else if(way.tags().has_key("highway")) {
      mType = HIGHWAY;
    }
    else if(way.tags().has_key("waterway")) {
      mType = WATER;
    }

    if(way.nodes().ends_have_same_id() && way.nodes().size() > 3) {
      mType |= CLOSED;
    }

    for(auto node : way.nodes()) {

      osmium::geom::Coordinates c = EngineBlock::CenterEarthFixedConvert::to_coords(node.location());

      glm::vec2 coord(c.x - ref.x, c.y - ref.y);

      worldCoords.push_back(coord);
    }

    if(worldCoords.size() == 0) {
      mValid = false;
      cout << "No valid points found " << mName << endl;
    }

  } catch (std::out_of_range) {
    static bool once = true;
    if(once) {
      cout << "Failed to decode certain buildings" << endl;
      once = false;
    }
  }
}


OSMFeature::OSMFeature(const osmium::Node& node, const WorldCoord& ref, bool findName)

: mType(LOCATION), mValid(true)

{
  const osmium::geom::Coordinates c = EngineBlock::CenterEarthFixedConvert::to_coords(node.location());

  glm::vec2 coord(c.x - ref.x, c.y - ref.y);

  worldCoords.push_back(coord);
}
