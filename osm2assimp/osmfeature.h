#pragma once

#include <vector>
#include <string>

#include <osmium/geom/coordinates.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/node.hpp>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

namespace GeoUtils {

using WorldCoord = osmium::geom::Coordinates;

// <summary>
// A class to hold the the 2d points of a single OSM feature,
// typically a building or road.
// This struct contains all the information needed to be converted into a mesh for Assimp later.   
// </summary>
class OSMFeature {

public:
  static const int UNDEFINED = 0;

  // the closed type refers to a collection of nodes that make up an area where the first and last nodes are the same.
  // As opposed to a sequence of points making a path or road, a spline.
  static const int CLOSED = 1; 
  static const int BUILDING = 2;
  static const int WATER = 4;
  static const int HIGHWAY = 8;
  static const int LOCATION = 16;

  static int determineTypeFromWay(const osmium::Way& way);
  
  // <summary>
  // Construct an OSMFeature created from an osmium way; a collection of nodes, eg road or building
  // <summary>
  OSMFeature(const osmium::Way& way, bool getNameFromOSM = false);

  // <summary>
  // Construct an OSMFeature created from a single node; eg a location or landmark
  // <summary>
  OSMFeature(const osmium::Node& node, bool getNameFromOSM = false);

  int type() const { return mType; }

  bool isValid() const { return mValid; }

  float height() const { return mHeight; }

  std::string name() const { return mName; }

  const std::vector<glm::vec2>& coords() const {
    return mWorldCoords;
  } 

  static int DefaultNumberOfFloors;
  static float BuildingFloorHeight;

private:

  float determineHeightFromWay(const osmium::Way& way);
  std::string getNameFromWay(const osmium::Way& way); 

  int mType;
  float mHeight;
  std::string mName;
  std::vector<glm::vec2> mWorldCoords;
  bool mValid;
};
  
}