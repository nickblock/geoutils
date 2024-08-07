#include "osmfeature.h"
#include <osmium/geom/mercator_projection.hpp>
#include <osmium/area/geom_assembler.hpp>

#include <sstream>

#include "convertlatlng.h"

using std::cout;
using std::endl;
using std::string;

namespace GeoUtils
{

  int OSMFeature::DefaultNumberOfFloors = 3;
  float OSMFeature::BuildingFloorHeight = 2.5f;
  float OSMFeature::RoadWidth = 3.0f;

  std::vector<std::vector<std::string>> NameTags = {
      {"name"}, {"addr:housename"}, {"addr:housenumber", "addr:street"}};

  float OSMFeature::determineHeightFromWay(const osmium::Way &way)
  {
    float height = BuildingFloorHeight * DefaultNumberOfFloors;

    if (way.tags().has_key("height"))
    {

      try
      {
        height = std::stof(way.tags()["height"]);
      }
      catch (std::invalid_argument)
      {
      }
    }
    else if (way.tags().has_key("building:levels"))
    {

      try
      {
        int floors = std::stoi(way.tags()["building:levels"]);
        height = floors * BuildingFloorHeight;
      }
      catch (std::invalid_argument)
      {
      }
    }
    return height;
  }

  std::string OSMFeature::getNameFromWay(const osmium::Way &way)
  {
    std::string name;
    for (const auto &taglist : NameTags)
    {
      bool foundAll = true;
      for (const auto &tag : taglist)
      {
        if (way.tags().has_key(tag.c_str()))
        {
          name += std::string(way.tags().get_value_by_key(tag.c_str(), "noname")) + std::string(" ");
        }
        else
        {
          foundAll = false;
          break;
        }
      }
      if (foundAll)
        break;

      else
      {
        name = std::string();
      }
    }
    if (name.size() == 0)
    {
      std::stringstream ss;
      ss << way.id();
      name = string(ss.str());
    }
    return name;
  }

  int OSMFeature::determineTypeFromWay(const osmium::Way &way)
  {
    int type = UNDEFINED;

    if (way.tags().has_key("building"))
    {
      type = BUILDING;
    }
    else if (way.tags().has_key("highway"))
    {
      type = HIGHWAY;
    }
    else if (way.tags().has_key("waterway"))
    {
      type = WATER;
    }

    if (way.nodes().size() > 3 && way.nodes().ends_have_same_id())
    {
      type |= CLOSED;
    }

    return type;
  }

  OSMFeature::OSMFeature(const std::vector<glm::vec2> &points, float height, int type, const std::string &name)
      : mWorldCoords(points),
        mType(type),
        mHeight(height),
        mName(name)
  {
    for (auto &p : mWorldCoords)
    {
      mBBox.add(glm::vec3(p, 0.0));
    }
  }

  OSMFeature::OSMFeature(const osmium::Way &way, bool getNameFromOSM)
      : mHeight(determineHeightFromWay(way)),
        mType(determineTypeFromWay(way)),
        mValid(true)
  {

    if (getNameFromOSM)
    {
      mName = getNameFromWay(way);
    }
    else
    {
      if (mType & HIGHWAY)
      {
        mName = "Highway_";
      }
      else if (mType & BUILDING)
      {
        mName = "Building_";
      }
      mName += std::to_string(way.id());
    }

    for (auto node : way.nodes())
    {

      osmium::geom::Coordinates c = ConvertLatLngToCoords::to_coords(node.location());

      glm::vec2 coord(c.x, c.y);

      mWorldCoords.push_back(coord);

      mBBox.add(glm::vec3(coord, 0.0));
    }

    if (
        (mType & HIGHWAY && mWorldCoords.size() < 2) ||
        (mType & BUILDING && !(mType & CLOSED)))
    {
      mValid = false;
      cout << "Not enough valid points found " << mName << endl;
    }
  }

  OSMFeature::OSMFeature(const osmium::Node &node, bool findName)

      : mType(LOCATION), mValid(true)

  {
    const osmium::geom::Coordinates c = ConvertLatLngToCoords::to_coords(node.location());

    glm::vec2 coord(c.x, c.y);

    mWorldCoords.push_back(coord);
  }

} // namespace GeoUtils