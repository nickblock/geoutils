#ifndef COMMON_GEO_H
#define COMMON_GEO_H

#include <osmium/geom/coordinates.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/node.hpp>

#include "glm/vec3.hpp"  
#include "glm/vec2.hpp"  
#include "glm/glm.hpp"  
#include "glm/gtc/constants.hpp"  

extern const float buildingFloorHeight;

using WorldCoord = osmium::geom::Coordinates;

struct Extents {

public:  
  Extents();
  Extents(std::string);

  bool valid() const;

  bool contains(const WorldCoord& c) const;

  bool contains(const osmium::Way& way) const;

  const WorldCoord& getMin() const {
    return min;
  }
  const WorldCoord& getMax() const {
    return max;
  }
  
protected:
  bool mValid;
  WorldCoord min, max;
};


#endif
