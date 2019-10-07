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

    if(mType != UNDEFINED && way.nodes().ends_have_same_id() && way.nodes().size() > 3) {
      
      mType |= CLOSED;

      try {

        osmium::area::AssemblerConfig area_config;
        area_config.ignore_invalid_locations = true;
        osmium::area::GeomAssembler assembler{area_config};
        osmium::memory::Buffer out_buffer{2048};

        if (!assembler(way, out_buffer)) {
            mValid = false;
        }

        osmium::geom::GEOSFactory<EngineBlock::CenterEarthFixedConvert> factory;
          
        const auto& area = out_buffer.get<osmium::Area>(0);

        //check for undefined items
        for (const auto& item : area) {
          if(item.type() == osmium::item_type::undefined) {
            cout << "Ignoring id " << mName << endl;
            mValid = false;
            return;
          }
        }

        // cout << "items = " << itemCount << endl;

        // mValid = false;
        // return;

        const std::unique_ptr<geos::geom::MultiPolygon> mp{factory.create_multipolygon(area)};
        
        if(mp->getNumGeometries()) {

          const geos::geom::Polygon* p0 = dynamic_cast<const geos::geom::Polygon*>(mp->getGeometryN(0));
          const geos::geom::LineString* l0e = p0->getExteriorRing();
          for(int i=0; i<l0e->getNumPoints(); i++) {
            const auto l0e_p0 = std::unique_ptr<geos::geom::Point>(l0e->getPointN(i));

            osmium::geom::Coordinates c{l0e_p0->getX(), l0e_p0->getY()};

            glm::vec2 coord(c.x - ref.x, c.y - ref.y);

            worldCoords.push_back(coord);
          }
        }
      }
      catch(osmium::geometry_error) {

        mValid = false;

        cout << "Geom Failed " << mName << endl;
      }
    }
    else {
      
      if(way.nodes().ends_have_same_id()) {
      
        mType |= CLOSED;
      }
      for(auto& node : way.nodes()) {

        const osmium::Location& location = node.location();

        if(!location.valid()) {
          mValid = false;
          cout << "Locator Failed " << mName << endl;
          break; 
        }

        const osmium::geom::Coordinates c = EngineBlock::CenterEarthFixedConvert::to_coords(location);

        glm::vec2 coord(c.x - ref.x, c.y - ref.y);

        worldCoords.push_back(coord);
      }
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
